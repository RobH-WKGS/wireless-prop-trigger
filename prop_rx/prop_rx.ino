// =====================================================================
//  Wireless Prop Trigger — RECEIVER (at the lighting desk)
// ---------------------------------------------------------------------
//  Board : iLabs Challenger RP2040 SubGHz 868 MHz (RFM69HCW, U.FL)
//  Core  : Earle Philhower "Raspberry Pi Pico/RP2040" (arduino-pico)
//          Tools > Board      > "iLabs Challenger 2040 SubGHz"
//          Tools > USB Stack  > "Adafruit TinyUSB"        <-- REQUIRED for USB MIDI
//          Tools > Flash Size > "8MB (Sketch: 1MB, FS: 7MB)"
//  Libs  : MIDI Library (FortySevenEffects). RFM69 (LowPowerLab, GPL-3) is bundled in src/RFM69
//          (TinyUSB, I2S, LittleFS ship with the core)
//
//  What it does
//   Core 0: listens for prop packets -> sends a USB MIDI note to Titan
//           and tells core 1 to start the matching WAV.
//   Core 1: streams 16-bit WAV from flash to an I2S DAC (line out to the
//           sound desk). Clocks silence while idle so a cue starts
//           within ~12 ms, not after a DAC wake-up.
//
//  Panel controls
//   ARM switch  (A0 -> GND = armed). DISARMED = SAFE: nothing goes to
//               Titan or the sound desk. Prop and GO presses only flicker
//               the TRIGGER LED, and the n / t test commands are refused.
//               Lets the actor test the prop in the wings.
//   GO button   (A1 -> GND). Operator backup: fires cue 1 (MIDI + audio)
//               when ARMED.
//   A cue can fire at most once per second (MIN_TRIGGER_GAP_MS), whatever
//   fires it — prop, GO, or both together never double-fire.
//
//  Two props: flash prop_tx with TX_NODE_ID 2 (PROP 1) and 3 (PROP 2).
//  Both fire the same cue; the 1 s limit stops a double fire if both are
//  pressed together.
//
//  Three plain 5 mm LEDs, each on its own pin through a 220 ohm resistor
//   SIGNAL 1 (green, D10) / SIGNAL 2 (green, D12) — link to prop 1 / prop 2
//     off          that prop not heard for 3 s (off / flat / out of range)
//     solid        connected, good signal
//     slow blink   connected, weak signal (RSSI below WEAK_RSSI)
//     fast blink   that prop's battery is low
//   TRIGGER (red, D11)
//     solid 1 s    cue fired (either prop, or GO)
//     flicker 1 s  a prop pressed while DISARMED (nothing fired)
//     2 blips / 3 s  NO USB HOST: no MIDI link (charge-only cable, PC off...)
//   All on for half a second at power-up (lamp test). All blinking together
//   fast = radio fault. ARM state shows on the flight switch.
//
//  Serial Monitor (any baud): h = help, m = MIDI/USB status,
//   n = send a test MIDI note (for Titan Learn; no audio, no lockout),
//   s = audio status, t = 1 kHz test tone. Every MIDI message in or out
//   is logged.
// =====================================================================

#include <SPI.h>
// RFM69 library (LowPowerLab v1.6.0, GPL-3) is bundled in src/RFM69, patched
// to build on the RP2040 core, which doesn't define SS for this board.
#include "src/RFM69/RFM69.h"
// USB MIDI needs the TinyUSB stack. Stop with a clear message if it's not selected.
#if !defined(USE_TINYUSB)
#error "Set Tools > USB Stack > Adafruit TinyUSB (it's on Pico SDK). Also remove any separately installed Adafruit TinyUSB library."
#endif
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <LittleFS.h>
#include <I2S.h>

// ---------- SHARED SETTINGS — must match prop_tx.ino ----------------
#define NETWORK_ID     77
#define RX_NODE_ID     1
#define FREQ_HZ        868300000UL
#define ENCRYPT_KEY    "ChangeMe16Chars!"

// ---------- MIDI -----------------------------------------------------
#define MIDI_CHANNEL   1
#define MIDI_BASE_NOTE 60          // cue 1 = note 60 (C4), cue 2 = 61 ...
#define NOTE_LENGTH_MS 100         // note-off follows after this
#define SEND_NOTE_OFF  true        // set false if Titan's key profile toggles on release

// ---------- Audio ----------------------------------------------------
#define ENABLE_AUDIO   true        // false = MIDI only (sound handled elsewhere)
#define I2S_BCLK_PIN   D5          // D5 (GPIO2) -> DAC BCK  (LRCLK is automatically GPIO3 = D6)
#define I2S_DATA_PIN   D9          // D9 (GPIO4) -> DAC DIN
#define AUDIO_DELAY_MS 0           // delay audio to line up with lamps if light lags sound
#define MAX_CUES       4           // files: /cue1.wav ... /cue4.wav (all the same sample rate)

// ---------- Panel ----------------------------------------------------
#define ARM_PIN        A0
#define GO_PIN         A1
#define SIGNAL1_LED_PIN D10        // PROP 1 (node 2): green LED (GPIO5) via 220 ohm to GND
#define SIGNAL2_LED_PIN D12        // PROP 2 (node 3): green LED (GPIO7) via 220 ohm to GND
#define TRIGGER_LED_PIN D11        // red LED (GPIO6) via 220 ohm to GND
#define PROP1_NODE      2
#define PROP2_NODE      3
#define LED_BRIGHTNESS  120        // 0-255 PWM; lower for a dark backstage
#define WEAK_RSSI       -90        // dBm; weaker than this = slow blink
#define TRIGGER_LED_MS 1000        // how long the TRIGGER LED stays lit
#define MIN_TRIGGER_GAP_MS 1000    // a cue can't fire again within this time

// ---------- Radio (built in on the Challenger, wired to SPI1) --------
#define RFM_CS         9           // RFM69HCW_SS
#define RFM_IRQ        14          // RFM69HCW_DIO0
#define RFM_RST        13          // RFM69HCW_RST
#define PROP_TIMEOUT_MS 3000
#define LOW_BATT_MV    3500

// ---------- Packet format (identical in transmitter) -----------------
enum : uint8_t { PKT_TRIGGER = 1, PKT_HEARTBEAT = 2 };
struct __attribute__((packed)) Packet {
  uint8_t  magic;
  uint8_t  type;
  uint8_t  cue;
  uint8_t  seq;
  uint16_t battMv;
};

RFM69 radio(RFM_CS, RFM_IRQ, true, &SPI1);
Adafruit_USBD_MIDI usbMidi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usbMidi, MIDI);

// ---------- State (core 0) -------------------------------------------
// WAV cue info (declared up here so the IDE's auto-generated prototypes can see it)
struct Cue {
  bool     present;
  uint16_t channels;
  uint32_t rate;
  uint32_t dataStart;
  uint32_t dataLen;
};
struct NodeState { uint32_t lastHeard; uint8_t lastSeq; uint32_t lastSeqAt; uint16_t battMv; int16_t rssi; };
NodeState nodes[8];                 // index = sender node id (2..7 used)
uint32_t noteOffAt[MAX_CUES + 1] = {0};
uint32_t lastFireAt[MAX_CUES + 1] = {0};
bool     everFired[MAX_CUES + 1] = {false};
uint32_t trigLedUntil = 0;
bool     trigLedFlicker = false;      // false = solid (fired), true = flicker (disarmed press)
uint32_t lastDisarmedFlash = 0;
bool     lastGo = HIGH;

// ---------- Shared between cores -------------------------------------
volatile uint8_t pendingCue = 0;    // written by core 0, cleared by core 1
volatile bool    audioReady = false;
volatile bool    toneRequest = false;  // play the 1 kHz test tone (core 1)
char             audioStatus[512] = "audio: not started";

// =====================================================================
//  CORE 0 — radio, MIDI, panel
// =====================================================================
// ---------- MIDI diagnostics ----------
uint32_t midiNotesSent = 0, midiInCount = 0, lastMidiSentAt = 0;
bool     usbWasMounted = false;

bool usbReady() { return TinyUSBDevice.mounted() && !TinyUSBDevice.suspended(); }

const char *noteName(uint8_t n) {
  static const char *names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  static char buf[8];
  snprintf(buf, sizeof(buf), "%s%d", names[n % 12], n / 12 - 1);
  return buf;
}

void midiNote(bool on, uint8_t note, uint8_t vel) {
  bool ok = usbReady();
  if (on) MIDI.sendNoteOn(note, vel, MIDI_CHANNEL);
  else    MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  if (on) { midiNotesSent++; lastMidiSentAt = millis(); }
  Serial.printf("[%lu] MIDI OUT note-%s ch %u note %u (%s) vel %u %s\n", millis(), on ? "on " : "off",
                MIDI_CHANNEL, note, noteName(note), on ? vel : 0,
                ok ? "[USB host connected]" : "[NOT DELIVERED: no USB host]");
}

void printMidiStatus() {
  Serial.printf("USB: %s%s\n", TinyUSBDevice.mounted() ? "connected to a host" : "NOT connected (charge-only cable, or PC not enumerating)",
                TinyUSBDevice.suspended() ? ", suspended (PC asleep?)" : "");
  Serial.printf("MIDI device name: \"Prop Trigger\". Sends ch %u, cue 1 = note %u (%s), note-off after %u ms%s\n",
                MIDI_CHANNEL, MIDI_BASE_NOTE, noteName(MIDI_BASE_NOTE), NOTE_LENGTH_MS, SEND_NOTE_OFF ? "" : " (note-off disabled)");
  Serial.printf("MIDI notes sent: %lu (last %lu ms ago), MIDI messages received: %lu\n", midiNotesSent,
                midiNotesSent ? millis() - lastMidiSentAt : 0, midiInCount);
}

void printHelp() {
  Serial.println("Commands: h help | m MIDI/USB status | n send test MIDI note | s audio status | t test tone");
}

bool isArmed() { return digitalRead(ARM_PIN) == LOW; }

void lightTrigger(bool flicker) {
  trigLedFlicker = flicker;
  trigLedUntil  = millis() + TRIGGER_LED_MS;
}

void fireCue(uint8_t cue, const char *source) {
  if (cue < 1 || cue > MAX_CUES) return;
  uint32_t now = millis();
  // At most one fire per second per cue, from any source
  if (everFired[cue] && (now - lastFireAt[cue]) < MIN_TRIGGER_GAP_MS) {
    Serial.printf("[%lu] cue %u from %s ignored (fired %lu ms ago)\n", now, cue, source, now - lastFireAt[cue]);
    return;
  }
  everFired[cue]  = true;
  lastFireAt[cue] = now;
  uint8_t note = MIDI_BASE_NOTE + cue - 1;
  midiNote(true, note, 127);
  noteOffAt[cue] = now + NOTE_LENGTH_MS;
  if (ENABLE_AUDIO) pendingCue = cue;
  lightTrigger(false);
  Serial.printf("[%lu] FIRE cue %u (note %u) from %s\n", now, cue, note, source);
}

void handlePacket(uint16_t sender, const Packet &p, int16_t rssi) {
  if (p.magic != 'W' || sender >= 8) return;
  NodeState &n = nodes[sender];
  uint32_t now = millis();
  n.lastHeard = now;
  n.rssi = rssi;
  if (p.battMv) n.battMv = p.battMv;

  if (p.type != PKT_TRIGGER) return;

  // De-duplicate the burst: same seq within 1 s is the same press.
  if (p.seq == n.lastSeq && (now - n.lastSeqAt) < 1000) return;
  n.lastSeq = p.seq;
  n.lastSeqAt = now;

  bool armed = isArmed();
  if (armed) {
    char src[32]; snprintf(src, sizeof(src), "PROP %u (node %u, RSSI %d)", sender == PROP2_NODE ? 2 : 1, sender, rssi);
    fireCue(p.cue, src);
  } else {
    // same 1-per-second limit applies to the test flash
    if (now - lastDisarmedFlash >= MIN_TRIGGER_GAP_MS) {
      lastDisarmedFlash = now;
      lightTrigger(true);
    }
    Serial.printf("[%lu] Press from prop %u ignored (DISARMED), RSSI %d\n", now, sender, rssi);
  }
}

void setLed(uint8_t pin, bool on) {
  static int8_t shown[32];                 // avoid re-writing PWM every loop
  int8_t v = on ? 1 : 0;
  if (shown[pin] == v + 1) return;
  shown[pin] = v + 1;
  analogWrite(pin, on ? LED_BRIGHTNESS : 0);
}

// SIGNAL pattern for one prop: off / solid / slow blink (weak) / fast blink (low battery)
bool signalFor(uint8_t node, uint32_t now) {
  NodeState &n = nodes[node];
  if (!n.lastHeard || (now - n.lastHeard) >= PROP_TIMEOUT_MS) return false;
  if (n.battMv && n.battMv < LOW_BATT_MV) return (now / 125) % 2;   // fast blink, 4 Hz
  if (n.rssi < WEAK_RSSI)                 return (now / 500) % 2;   // slow blink, 1 Hz
  return true;
}

void updateLeds() {
  uint32_t now = millis();
  setLed(SIGNAL1_LED_PIN, signalFor(PROP1_NODE, now));
  setLed(SIGNAL2_LED_PIN, signalFor(PROP2_NODE, now));

  bool trig = false;
  if ((int32_t)(trigLedUntil - now) > 0) {
    trig = trigLedFlicker ? (now / 60) % 2 : true;
  } else if (!TinyUSBDevice.mounted()) {
    uint32_t ph = now % 3000;                // no USB host: two short blips every 3 s
    trig = (ph < 80) || (ph >= 250 && ph < 330);
  }
  setLed(TRIGGER_LED_PIN, trig);
}

void setup() {
  // USB first so Windows sees a MIDI device straight away
  if (!TinyUSBDevice.isInitialized()) TinyUSBDevice.begin(0);
  TinyUSBDevice.setProductDescriptor("Prop Trigger");
  usbMidi.setStringDescriptor("Prop Trigger MIDI");
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  if (TinyUSBDevice.mounted()) { TinyUSBDevice.detach(); delay(10); TinyUSBDevice.attach(); }

  Serial.begin(115200);
  pinMode(ARM_PIN, INPUT_PULLUP);
  pinMode(GO_PIN, INPUT_PULLUP);

  const uint8_t ledPins[] = {SIGNAL1_LED_PIN, SIGNAL2_LED_PIN, TRIGGER_LED_PIN};
  for (uint8_t pin : ledPins) { pinMode(pin, OUTPUT); analogWrite(pin, LED_BRIGHTNESS); }  // lamp test
  delay(500);
  for (uint8_t pin : ledPins) analogWrite(pin, 0);

  pinMode(RFM_RST, OUTPUT);
  digitalWrite(RFM_RST, HIGH); delay(10);
  digitalWrite(RFM_RST, LOW);  delay(10);

  if (!radio.initialize(RF69_868MHZ, RX_NODE_ID, NETWORK_ID)) {
    while (true) {                                   // radio fault: both LEDs blink together
      int v = (millis() / 100) % 2 ? LED_BRIGHTNESS : 0;
      analogWrite(SIGNAL1_LED_PIN, v); analogWrite(SIGNAL2_LED_PIN, v); analogWrite(TRIGGER_LED_PIN, v);
      delay(50);
    }
  }
  radio.setFrequency(FREQ_HZ);
  radio.setHighPower(true);
  radio.encrypt(ENCRYPT_KEY);
  radio.receiveDone();                               // enter RX mode
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif
  // Drain incoming MIDI (and log it: handy to confirm the PC can talk to us)
  while (MIDI.read()) {
    midiInCount++;
    Serial.printf("[%lu] MIDI IN type 0x%02X ch %u data %u %u\n", millis(), (unsigned)MIDI.getType(),
                  MIDI.getChannel(), MIDI.getData1(), MIDI.getData2());
  }
  // Log USB connect / disconnect
  bool mounted = TinyUSBDevice.mounted();
  if (mounted != usbWasMounted) {
    usbWasMounted = mounted;
    Serial.printf("[%lu] USB %s\n", millis(), mounted ? "connected to host: MIDI ready"
                                                        : "disconnected: no MIDI link");
  }

  // Serial Monitor commands (see printHelp)
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'h' || c == 'H' || c == '?') printHelp();
    else if (c == 'm' || c == 'M') printMidiStatus();
    else if (c == 'n' || c == 'N') {             // test note for Titan Learn: no audio, no lockout
      if (!isArmed()) Serial.println("test MIDI note refused: DISARMED (flip ARM to send)");
      else {
        Serial.println("test MIDI note (no audio):");
        midiNote(true, MIDI_BASE_NOTE, 127);
        noteOffAt[1] = millis() + NOTE_LENGTH_MS;
      }
    }
    else if (c == 's' || c == 'S') { Serial.print(audioStatus); Serial.println(); }
    else if (c == 't' || c == 'T') {
      if (!isArmed())      Serial.println("test tone refused: DISARMED (flip ARM to play)");
      else if (audioReady) { toneRequest = true; Serial.println("test tone: 1 kHz for 2 s"); }
      else            { Serial.print("test tone unavailable: "); Serial.print(audioStatus); Serial.println(); }
    }
  }

  if (radio.receiveDone()) {
    uint16_t sender = radio.SENDERID;               // 16-bit in RFM69 lib v1.5+
    int16_t rssi   = radio.RSSI;
    Packet p; bool ok = (radio.DATALEN == sizeof(Packet));
    if (ok) memcpy(&p, (const void *)radio.DATA, sizeof(Packet));
    radio.receiveDone();                             // straight back to RX
    if (ok) handlePacket(sender, p, rssi);
  }

  // Operator backup GO button (always live)
  bool g = digitalRead(GO_PIN);
  if (g == LOW && lastGo == HIGH) {
    if (isArmed()) fireCue(1, "panel GO");               // fireCue enforces the 1 s limit
    else {
      if (millis() - lastDisarmedFlash >= MIN_TRIGGER_GAP_MS) {
        lastDisarmedFlash = millis();
        lightTrigger(true);                              // same flicker as a disarmed prop press
      }
      Serial.printf("[%lu] GO ignored (DISARMED)\n", millis());
    }
  }
  lastGo = g;

  uint32_t now = millis();
  for (int c = 1; c <= MAX_CUES; c++) {
    if (noteOffAt[c] && (int32_t)(now - noteOffAt[c]) >= 0) {
      if (SEND_NOTE_OFF) midiNote(false, MIDI_BASE_NOTE + c - 1, 0);
      noteOffAt[c] = 0;
    }
  }

  updateLeds();

  // Every 2 s, print link quality and USB state to the serial monitor
  static uint32_t lastReport = 0;
  if (now - lastReport > 2000) {
    lastReport = now;
    char line[160]; size_t k = 0;
    for (int p = 1; p <= 2; p++) {
      NodeState &n = nodes[p == 1 ? PROP1_NODE : PROP2_NODE];
      if (n.lastHeard && now - n.lastHeard < PROP_TIMEOUT_MS)
        k += snprintf(line + k, sizeof(line) - k, "PROP %d: %d dBm %u mV | ", p, n.rssi, n.battMv);
      else
        k += snprintf(line + k, sizeof(line) - k, "PROP %d: -- | ", p);
    }
    snprintf(line + k, sizeof(line) - k, "USB %s, %lu notes sent", TinyUSBDevice.mounted() ? "ok" : "NO HOST", midiNotesSent);
    Serial.println(line);
  }
}

// =====================================================================
//  CORE 1 — WAV playback to I2S
// =====================================================================
Cue cues[MAX_CUES + 1];
I2S i2s(OUTPUT);
uint32_t audioRate = 44100;
File playing;
uint32_t bytesLeft = 0;
uint16_t playChannels = 1;
uint8_t  blockBuf[1024];

bool parseWav(File &f, Cue &c) {
  uint8_t h[12];
  if (f.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
  bool haveFmt = false;
  while (f.available()) {
    uint8_t ch[8];
    if (f.read(ch, 8) != 8) return false;
    uint32_t len = ch[4] | (ch[5] << 8) | (ch[6] << 16) | ((uint32_t)ch[7] << 24);
    if (!memcmp(ch, "fmt ", 4)) {
      uint8_t fmt[16];
      if (len < 16 || f.read(fmt, 16) != 16) return false;
      uint16_t format = fmt[0] | (fmt[1] << 8);
      c.channels = fmt[2] | (fmt[3] << 8);
      c.rate     = fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | ((uint32_t)fmt[7] << 24);
      uint16_t bits = fmt[14] | (fmt[15] << 8);
      if (format != 1 || bits != 16 || c.channels < 1 || c.channels > 2) return false;
      f.seek(f.position() + (len - 16) + (len & 1));
      haveFmt = true;
    } else if (!memcmp(ch, "data", 4)) {
      if (!haveFmt) return false;
      c.dataStart = f.position();
      c.dataLen   = len;
      return true;
    } else {
      f.seek(f.position() + len + (len & 1));
    }
  }
  return false;
}

void setup1() {
  if (!ENABLE_AUDIO) { snprintf(audioStatus, sizeof(audioStatus), "audio: disabled (ENABLE_AUDIO false)"); return; }
  pinMode(GO_PIN, INPUT_PULLUP);
  while (!Serial && millis() < 1500) delay(10);      // let core 0 bring USB up first

  size_t n = 0;
  uint32_t rate = 0;
  if (!LittleFS.begin()) {
    n += snprintf(audioStatus + n, sizeof(audioStatus) - n, "audio: LittleFS mount failed (upload the data folder)\n");
  } else {
    // list everything on the flash, so a wrongly named or missing file is obvious
    n += snprintf(audioStatus + n, sizeof(audioStatus) - n, "flash files:");
    int count = 0;
    Dir dir = LittleFS.openDir("/");
    while (dir.next() && n < sizeof(audioStatus) - 40) {
      n += snprintf(audioStatus + n, sizeof(audioStatus) - n, " %s", dir.fileName().c_str());
      count++;
    }
    n += snprintf(audioStatus + n, sizeof(audioStatus) - n, count ? "\n" : " (none: data folder not uploaded)\n");
    for (int i = 1; i <= MAX_CUES; i++) {
      char name[16]; snprintf(name, sizeof(name), "/cue%d.wav", i);
      cues[i].present = false;
      File f = LittleFS.open(name, "r");
      if (!f) continue;
      if (parseWav(f, cues[i]) && (rate == 0 || cues[i].rate == rate)) {
        cues[i].present = true;
        rate = cues[i].rate;
        n += snprintf(audioStatus + n, sizeof(audioStatus) - n, "%s: %lu Hz, %u ch, %.2f s\n", name, cues[i].rate,
                      cues[i].channels, cues[i].dataLen / (2.0f * cues[i].channels * cues[i].rate));
      } else {
        n += snprintf(audioStatus + n, sizeof(audioStatus) - n, "%s: SKIPPED (needs 16-bit PCM WAV, same rate as others)\n", name);
      }
      f.close();
    }
  }
  if (!rate) {
    n += snprintf(audioStatus + n, sizeof(audioStatus) - n, "audio: NO cue WAVs found; test tone still works\n");
    rate = 44100;                                    // start I2S anyway so the test tone works
  }

  i2s.setBCLK(I2S_BCLK_PIN);
  i2s.setDATA(I2S_DATA_PIN);
  i2s.setBitsPerSample(16);
  i2s.setBuffers(4, 128);                            // ~12 ms of buffering at 44.1 kHz
  if (!i2s.begin(rate)) {
    snprintf(audioStatus + n, sizeof(audioStatus) - n, "audio: I2S start FAILED\n");
    return;
  }
  snprintf(audioStatus + n, sizeof(audioStatus) - n, "audio: I2S running at %lu Hz\n", rate);
  audioRate = rate;
  audioReady = true;
  Serial.print(audioStatus);
  pinMode(ARM_PIN, INPUT_PULLUP);
  if (digitalRead(GO_PIN) == LOW && digitalRead(ARM_PIN) == LOW) toneRequest = true;  // ARMED + GO held at power-up = test tone
}

void loop1() {
  if (!audioReady) { pendingCue = 0; delay(5); return; }

  if (toneRequest) {                       // 2 s, 1 kHz, about -6 dBFS, both channels
    toneRequest = false;
    bytesLeft = 0;
    if (playing) playing.close();
    uint32_t total = audioRate * 2;
    uint32_t phase = 0, step = (uint32_t)((1000.0 * 4294967296.0) / audioRate);
    for (uint32_t i = 0; i < total; i++) {
      int16_t v = (int16_t)(16000.0f * sinf(phase * (6.2831853f / 4294967296.0f)));
      i2s.write16(v, v);
      phase += step;
      if (pendingCue) break;                 // a real cue interrupts the test tone
    }
  }

  uint8_t cue = pendingCue;
  if (cue) {
    pendingCue = 0;
    if (cue <= MAX_CUES && cues[cue].present) {
      uint32_t startAt = millis() + AUDIO_DELAY_MS;
      while ((int32_t)(millis() - startAt) < 0) i2s.write16(0, 0);
      if (playing) playing.close();
      char name[16]; snprintf(name, sizeof(name), "/cue%u.wav", cue);
      playing = LittleFS.open(name, "r");
      if (playing) {
        playing.seek(cues[cue].dataStart);
        bytesLeft    = cues[cue].dataLen;
        playChannels = cues[cue].channels;
      }
    }
  }

  if (bytesLeft && playing) {
    size_t want = bytesLeft < sizeof(blockBuf) ? bytesLeft : sizeof(blockBuf);
    size_t got  = playing.read(blockBuf, want);
    if (got == 0) { bytesLeft = 0; playing.close(); return; }
    bytesLeft -= got;
    int16_t *s = (int16_t *)blockBuf;
    size_t samples = got / 2;
    if (playChannels == 1) {
      for (size_t i = 0; i < samples; i++) i2s.write16(s[i], s[i]);
    } else {
      for (size_t i = 0; i + 1 < samples; i += 2) i2s.write16(s[i], s[i + 1]);
    }
    if (!bytesLeft) playing.close();
  } else {
    // Idle: keep clocking silence so the next cue starts instantly.
    for (int i = 0; i < 64; i++) i2s.write16(0, 0);
  }
}
