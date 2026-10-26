// =====================================================================
//  Wireless Prop Trigger — TRANSMITTER (lives inside the prop)
// ---------------------------------------------------------------------
//  Board : iLabs Challenger RP2040 SubGHz 868 MHz (RFM69HCW, U.FL)
//  Core  : Earle Philhower "Raspberry Pi Pico/RP2040" (arduino-pico)
//          Tools > Board     > "iLabs Challenger 2040 SubGHz"
//          Tools > CPU Speed > 50 MHz  (saves battery, plenty fast)
//  Libs  : RFM69 by LowPowerLab (v1.5+)
//
//  Behaviour
//   * Button press -> sends a burst of identical TRIGGER packets
//     (same sequence number) so one lost packet doesn't matter.
//   * Sends a HEARTBEAT once a second (with battery voltage) so the
//     receiver can show the prop is alive before the scene.
//   * Radio sleeps between sends. MCU stays awake so a press is sent
//     instantly (no wake-from-sleep delay).
// =====================================================================

#include <SPI.h>
#include <RFM69.h>

#if !defined(ARDUINO_CHALLENGER_2040_SUBGHZ_RP2040)
  #warning "Written for the iLabs Challenger 2040 SubGHz — check the radio pin defines below"
#endif

// ---------- SHARED SETTINGS — must match prop_rx.ino ----------------
#define NETWORK_ID     77
#define RX_NODE_ID     1
#define FREQ_HZ        868300000UL      // 868.0–868.6 MHz sub-band (25 mW, 1% duty)
#define ENCRYPT_KEY    "ChangeMe16Chars!" // EXACTLY 16 characters
#define TX_POWER_DBM   10               // 10 dBm is ample for 6 m; max legal here is 14

// ---------- THIS PROP ------------------------------------------------
#define TX_NODE_ID     2                // 2 = main prop, 3 = spare prop
#define CUE_NUMBER     1                // -> MIDI note 60 at the receiver
#define BUTTON_PIN     D5               // momentary button between D5 and GND
#define BATT_PIN       A2               // 100k/100k divider from BAT; set to -1 if not fitted

#define BURST_COUNT    4                // copies of each trigger packet
#define BURST_GAP_MS   3
#define LOCKOUT_MS     300              // ignore bounce / double-press for this long
#define HEARTBEAT_MS   1000

// ---------- Radio (built in on the Challenger, wired to SPI1) --------
#define RFM_CS         9                // RFM69HCW_SS
#define RFM_IRQ        14               // RFM69HCW_DIO0
#define RFM_RST        13               // RFM69HCW_RST
RFM69 radio(RFM_CS, RFM_IRQ, true /* HCW high-power module */, &SPI1);

// ---------- Packet format (identical in receiver) --------------------
enum : uint8_t { PKT_TRIGGER = 1, PKT_HEARTBEAT = 2 };
struct __attribute__((packed)) Packet {
  uint8_t  magic;    // 'W'
  uint8_t  type;     // PKT_*
  uint8_t  cue;      // cue number
  uint8_t  seq;      // increments per press
  uint16_t battMv;   // battery millivolts (0 = not measured)
};

uint8_t  seq        = 0;
uint16_t battMv     = 0;
uint32_t lastPress  = 0;
uint32_t lastBeat   = 0;
bool     lastButton = HIGH;

uint16_t readBattMv() {
  if (BATT_PIN < 0) return 0;
  uint32_t raw = 0;
  for (int i = 0; i < 8; i++) raw += analogRead(BATT_PIN);
  raw /= 8;
  return (uint16_t)(raw * 3300UL * 2UL / 4095UL);   // x2 for the 1:2 divider
}

void sendPacket(uint8_t type) {
  Packet p = { 'W', type, CUE_NUMBER, seq, battMv };
  radio.send(RX_NODE_ID, &p, sizeof(p), false);      // no ACK: receiver never transmits
}

void failForever() {
  // Radio didn't start: fast blink so you notice on the bench.
  while (true) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(80); }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);                    // keep the prop dark on stage
  analogReadResolution(12);

  // Hard-reset the radio module (RST is active-high on RFM69)
  pinMode(RFM_RST, OUTPUT);
  digitalWrite(RFM_RST, HIGH); delay(10);
  digitalWrite(RFM_RST, LOW);  delay(10);

  if (!radio.initialize(RF69_868MHZ, TX_NODE_ID, NETWORK_ID)) failForever();
  radio.setFrequency(FREQ_HZ);
  radio.setHighPower(true);
  radio.setPowerDBm(TX_POWER_DBM);
  radio.encrypt(ENCRYPT_KEY);
  radio.sleep();

  battMv = readBattMv();

  // Two quick blinks = booted OK, then LED stays off.
  for (int i = 0; i < 2; i++) { digitalWrite(LED_BUILTIN, HIGH); delay(60); digitalWrite(LED_BUILTIN, LOW); delay(120); }
}

void loop() {
  uint32_t now = millis();
  bool b = digitalRead(BUTTON_PIN);

  // Fire on the very first falling edge — no debounce delay before sending.
  if (b == LOW && lastButton == HIGH && (now - lastPress) > LOCKOUT_MS) {
    lastPress = now;
    seq++;
    for (int i = 0; i < BURST_COUNT; i++) {
      sendPacket(PKT_TRIGGER);
      delay(BURST_GAP_MS);
    }
    radio.sleep();
    lastBeat = now;                                  // no heartbeat straight after a trigger
  }
  lastButton = b;

  if (now - lastBeat >= HEARTBEAT_MS) {
    battMv = readBattMv();
    sendPacket(PKT_HEARTBEAT);
    radio.sleep();
    lastBeat = now;
  }
}
