// =====================================================================
//  Wireless Prop Trigger — RECEIVER (at the lighting desk)
// ---------------------------------------------------------------------
//  Board : iLabs Challenger RP2040 SubGHz 868 MHz (RFM69HCW, U.FL)
//  Core  : Earle Philhower "Raspberry Pi Pico/RP2040" (arduino-pico)
//          Tools > Board      > "iLabs Challenger 2040 SubGHz"
//          Tools > USB Stack  > "Adafruit TinyUSB"        <-- REQUIRED for USB MIDI
//          Tools > Flash Size > "8MB (Sketch: 1MB, FS: 7MB)"
//  Libs  : RFM69 (LowPowerLab), MIDI Library (FortySevenEffects),
//          Adafruit NeoPixel  (TinyUSB, I2S, LittleFS ship with the core)
//
//  What it does
//   Core 0: listens for prop packets -> sends a USB MIDI note to Titan
//           and tells core 1 to start the matching WAV.
//   Core 1: streams 16-bit WAV from flash to an I2S DAC (line out to the
//           sound desk). Clocks silence while idle so a cue starts
//           within ~12 ms, not after a DAC wake-up.
//
//  Panel controls
//   ARM switch  (A0 -> GND = armed). Disarmed: prop presses are shown on
//               the status LED (magenta flash) but fire NOTHING — lets the
//               actor test the prop in the wings.
//   GO button   (A1 -> GND). Operator backup: fires cue 1 (MIDI + audio)
//               whether armed or not.
//
//  Status NeoPixel (external, on D10 — the Challenger has no onboard one)
//   red       no prop heard for 3 s (off / flat battery / out of range)
//   green      prop alive, ARMED
//   blue       prop alive, disarmed (test mode)
//   amber blink prop battery low
//   white      cue fired           magenta  press received while disarmed
// =====================================================================

#include <SPI.h>
#include <RFM69.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
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
#define STATUS_PIXEL_PIN D10       // external single NeoPixel / WS2812 data in

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
Adafruit_NeoPixel pixel(1, STATUS_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ---------- State (core 0) -------------------------------------------
struct NodeState { uint32_t lastHeard; uint8_t lastSeq; uint32_t lastSeqAt; uint16_t battMv; int16_t rssi; };
NodeState nodes[8];                 // index = sender node id (2..7 used)
uint32_t noteOffAt[MAX_CUES + 1] = {0};
uint32_t flashUntil = 0;
uint32_t flashColour = 0;
bool     lastGo = HIGH;
uint32_t lastGoAt = 0;

// ---------- Shared between cores -------------------------------------
volatile uint8_t pendingCue = 0;    // written by core 0, cleared by core 1
volatile bool    audioReady = false;

// =====================================================================
//  CORE 0 — radio, MIDI, panel
// =====================================================================
void fireCue(uint8_t cue, const char *source) {
  if (cue < 1 || cue > MAX_CUES) return;
  uint8_t note = MIDI_BASE_NOTE + cue - 1;
  MIDI.sendNoteOn(note, 127, MIDI_CHANNEL);
  noteOffAt[cue] = millis() + NOTE_LENGTH_MS;
  if (ENABLE_AUDIO) pendingCue = cue;
  flashColour = pixel.Color(255, 255, 255);
  flashUntil  = millis() + 250;
  Serial.printf("[%lu] FIRE cue %u (note %u) from %s\n", millis(), cue, note, source);
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

  bool armed = (digitalRead(ARM_PIN) == LOW);
  if (armed) {
    char src[24]; snprintf(src, sizeof(src), "prop %u (RSSI %d)", sender, rssi);
    fireCue(p.cue, src);
  } else {
    flashColour = pixel.Color(255, 0, 255);
    flashUntil  = now + 250;
    Serial.printf("[%lu] Press from prop %u ignored (DISARMED), RSSI %d\n", now, sender, rssi);
  }
}

void updateStatusLed() {
  uint32_t now = millis();
  uint32_t c;
  if (now < flashUntil) {
    c = flashColour;
  } else {
    bool alive = false, lowBatt = false;
    for (int i = 2; i < 8; i++) {
      if (nodes[i].lastHeard && (now - nodes[i].lastHeard) < PROP_TIMEOUT_MS) {
        alive = true;
        if (nodes[i].battMv && nodes[i].battMv < LOW_BATT_MV) lowBatt = true;
      }
    }
    bool armed = (digitalRead(ARM_PIN) == LOW);
    if (!alive)                          c = pixel.Color(255, 0, 0);
    else if (lowBatt && (now / 400) % 2) c = pixel.Color(255, 120, 0);
    else if (armed)                      c = pixel.Color(0, 255, 0);
    else                                 c = pixel.Color(0, 0, 255);
  }
  static uint32_t shown = 1;
  if (c != shown) { pixel.setPixelColor(0, c); pixel.show(); shown = c; }
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

  pixel.begin();
  pixel.setBrightness(25);                           // backstage-friendly
  pixel.setPixelColor(0, pixel.Color(255, 0, 0)); pixel.show();

  pinMode(RFM_RST, OUTPUT);
  digitalWrite(RFM_RST, HIGH); delay(10);
  digitalWrite(RFM_RST, LOW);  delay(10);

  if (!radio.initialize(RF69_868MHZ, RX_NODE_ID, NETWORK_ID)) {
    while (true) {                                   // radio fault: fast red/off blink
      pixel.setPixelColor(0, (millis() / 100) % 2 ? pixel.Color(255, 0, 0) : 0); pixel.show();
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
  MIDI.read();                                       // keep the USB MIDI endpoint drained

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
  if (g == LOW && lastGo == HIGH && millis() - lastGoAt > 300) {
    lastGoAt = millis();
    fireCue(1, "panel GO");
  }
  lastGo = g;

  uint32_t now = millis();
  for (int c = 1; c <= MAX_CUES; c++) {
    if (noteOffAt[c] && (int32_t)(now - noteOffAt[c]) >= 0) {
      if (SEND_NOTE_OFF) MIDI.sendNoteOff(MIDI_BASE_NOTE + c - 1, 0, MIDI_CHANNEL);
      noteOffAt[c] = 0;
    }
  }

  updateStatusLed();

  // Once a second, print link quality to the serial monitor
  static uint32_t lastReport = 0;
  if (now - lastReport > 1000) {
    lastReport = now;
    for (int i = 2; i < 8; i++) if (nodes[i].lastHeard && now - nodes[i].lastHeard < PROP_TIMEOUT_MS)
      Serial.printf("prop %d: RSSI %d dBm, batt %u mV\n", i, nodes[i].rssi, nodes[i].battMv);
  }
}

// =====================================================================
//  CORE 1 — WAV playback to I2S
// =====================================================================
struct Cue {
  bool     present;
  uint16_t channels;
  uint32_t rate;
  uint32_t dataStart;
  uint32_t dataLen;
};
Cue cues[MAX_CUES + 1];
I2S i2s(OUTPUT);
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
  if (!ENABLE_AUDIO) return;
  while (!Serial && millis() < 1500) delay(10);      // let core 0 bring USB up first
  if (!LittleFS.begin()) { Serial.println("LittleFS mount failed — upload the data folder"); return; }

  uint32_t rate = 0;
  for (int i = 1; i <= MAX_CUES; i++) {
    char name[16]; snprintf(name, sizeof(name), "/cue%d.wav", i);
    cues[i].present = false;
    File f = LittleFS.open(name, "r");
    if (!f) continue;
    if (parseWav(f, cues[i]) && (rate == 0 || cues[i].rate == rate)) {
      cues[i].present = true;
      rate = cues[i].rate;
      Serial.printf("%s: %lu Hz, %u ch, %.2f s\n", name, cues[i].rate, cues[i].channels,
                    cues[i].dataLen / (2.0f * cues[i].channels * cues[i].rate));
    } else {
      Serial.printf("%s: skipped (needs 16-bit PCM WAV, same sample rate as others)\n", name);
    }
    f.close();
  }
  if (!rate) { Serial.println("No cue WAVs found — MIDI only"); return; }

  i2s.setBCLK(I2S_BCLK_PIN);
  i2s.setDATA(I2S_DATA_PIN);
  i2s.setBitsPerSample(16);
  i2s.setBuffers(4, 128);                            // ~12 ms of buffering at 44.1 kHz
  if (!i2s.begin(rate)) { Serial.println("I2S start failed"); return; }
  audioReady = true;
}

void loop1() {
  if (!audioReady) { pendingCue = 0; delay(5); return; }

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
