# Wireless Prop Trigger

A button in a handheld prop fires a lighting cue in Titan and a sound effect together.

```
 [Prop]                                 [Receiver at lighting PC]
 Challenger RP2040 SubGHz  ── 868 MHz ──▶ Challenger RP2040 SubGHz
 button + LiPo + FPC antenna               ├─ USB MIDI note ──▶ Titan (T3) trigger ─▶ lighting cue
                                           └─ I2S DAC line out ─▶ isolator ─▶ sound desk
```

Every unit uses the same board, the **iLabs Challenger RP2040 SubGHz 868 MHz**. It's a Feather-format board with an RP2040, an RFM69HCW radio, a LiPo charger and a U.FL antenna connector. Any board can be flashed as a prop or as the receiver, so one spare covers everything.

- **Radio:** RFM69 FSK at 868.3 MHz with AES-128 encryption. It's well clear of the 2.4 GHz phone and Wi-Fi traffic.
- **Reliability:** each press sends 4 identical packets. The receiver de-duplicates them, so one lost packet changes nothing.
- **Latency:** press to MIDI and audio is typically under 20 ms. Titan to DMX to fixtures then adds its own delay. `AUDIO_DELAY_MS` lets you hold the sound back to match the lamps if needed.
- **Heartbeat:** the prop reports once a second, with battery voltage. The receiver's LED tells you it's alive *before* the scene.
- **Safety:** an ARM switch on the receiver means nothing fires while you're testing the prop in the wings. A GO button on the receiver is the operator's backup.

> ⚠️ **Not yet compiled.** I checked the library calls and the board's pin map against their source, but couldn't run the compiler in my environment. Expect possibly a small fix or two on the first compile. Bench-test before rehearsal.

---

## Parts to purchase

UK stockist: The Pi Hut. Prices change, so check at checkout.

### Core (required)

| # | Item | Qty | Notes |
|---|------|-----|-------|
| 1 | **iLabs Challenger RP2040 SubGHz, 868 MHz, U.FL** | **4** | 2 props (main and spare), 1 receiver, 1 cold spare. About £16 each. **Buy the 868 MHz version**, not the 915 MHz one. Loose headers are included. |
| 2 | **868 MHz U.FL flexible PCB (FPC) antenna**, adhesive | 2–3 | One per prop. Flat, sticks inside the prop shell. Must say 868 MHz (or 860–930 MHz). |
| 3 | **U.FL to SMA bulkhead pigtail** (~10–15 cm) | 1 | Receiver: brings the antenna outside the box. |
| 4 | **868 MHz SMA whip / rubber-duck antenna** | 1 | Receiver. Mount it high, with line of sight to the stage. |
| 5 | 3.7 V LiPo, **2.0 mm JST-PH 2-pin**, ~400–500 mAh | 2 | One per prop. About 25 h runtime. **Check polarity against the board's + and − marks**: cells vary between sellers. |
| 6 | Momentary push button for the prop (e.g. 6–12 mm tactile or 7 mm panel push) | 2 | Pick to suit the prop. Normally-open. |
| 7 | Miniature slide switch (SPST) | 2 | Prop power. See wiring notes. |
| 8 | 100 kΩ resistor | 4 | 2 per prop, as a battery-voltage divider. |
| 8a | 100 nF ceramic capacitor | 2 | Optional, one per prop, across R2 to steady the battery reading. |
| 9 | **PCM5102A I2S DAC module** (common "GY-PCM5102" blue board) | 1 (+1 spare) | True line-level stereo out on a 3.5 mm jack. See jumper note below. |
| 10 | **Line-level isolator / DI** (e.g. Behringer HD400, ART DTI, or a passive DI box) | 1 | Breaks the USB-to-sound-desk ground loop. Don't skip this one. |
| 11 | 3.5 mm stereo to 2× ¼" jack or 2× XLR cable (to suit the isolator) | 1 | Line out to the isolator. |
| 12 | USB-C **data** cable, 1–2 m | 1 | Receiver to lighting PC. A charge-only cable won't enumerate. |

### Receiver panel

| # | Item | Qty | Notes |
|---|------|-----|-------|
| 13 | **Single NeoPixel / WS2812B** (e.g. Adafruit NeoPixel Mini PCB, or a 5 mm through-hole addressable LED) | 1 (+1 spare) | Status LED. The Challenger has no onboard NeoPixel. |
| 14 | Small project box (~100 × 60 × 30 mm) | 1 | Or 3D-print one. |
| 15 | Toggle switch, SPST (ideally with a flip-up safety cover) | 1 | ARM. |
| 16 | Momentary push button, 16–22 mm (arcade or panel) | 1 | Backup GO. |
| 17 | Female headers and standoffs | 1 set | Optional, so the board is removable. |

### Nice to have

- **Spare charged LiPo** for each prop, kept in the prompt corner.
- **USB-C power bank** to charge props between shows.
- The **4th Challenger**, pre-flashed as a receiver and kept as a cold spare. Reflash it as a prop if you need one.

---

## Wiring

The radio is already wired internally on SPI1 (CS GPIO9, DIO0 GPIO14, RST GPIO13). **Don't use D14–D17 (GPIO 13/14/15/18) for anything else**, because those are radio signals.

### Prop

![Prop transmitter schematic](docs/prop_tx_schematic.svg)

(PNG version: [`docs/prop_tx_schematic.png`](docs/prop_tx_schematic.png). Regenerate with `pip install schemdraw` then `python docs/prop_tx_schematic.py docs/prop_tx_schematic.svg`.)

| Board pin | Connects to |
|---|---|
| **D5** | Button, other leg to **GND** (internal pull-up used) |
| **BAT** | 100 k → **A2** → 100 k → **GND** (battery monitor) |
| U.FL | FPC antenna, stuck flat on the inside of the shell, **away from where fingers grip** |
| JST | LiPo |
| Power switch | If the board has an **EN** pin, slide switch between **EN and GND** (closed = off; charging still works). If not, put the switch in the LiPo's + lead. |

The green user LED is left off in code so the prop stays dark on stage. The red CHG LED only lights on USB. Cover the LEDs with tape if the prop is translucent.

Skip the divider if you like: set `BATT_PIN` to `-1` in `prop_tx.ino`. The receiver then won't warn about low battery.

### Receiver

| Board pin | Connects to |
|---|---|
| **D5** | DAC **BCK** |
| **D6** | DAC **LCK** (LRCLK, which the code drives as BCLK+1) |
| **D9** | DAC **DIN** |
| **3V3** | DAC **VIN** and NeoPixel **+** |
| **GND** | DAC **GND** and NeoPixel **−** |
| **D10** | NeoPixel **DIN** |
| **A0** | ARM switch, other leg to **GND** (closed = armed) |
| **A1** | GO button, other leg to **GND** |
| U.FL | Bulkhead SMA pigtail and 868 MHz antenna |

**PCM5102A jumpers:** most GY-PCM5102 boards need **SCK tied to GND**, either via the solder pad or the SCK pin. Then use the solder pads on the back to set **FLT, DEMP and FMT low and XSMT high**. Check your board's silkscreen, because the pad numbering varies between sellers. If you get no audio or a buzz, this is the first thing to check.

---

## Software setup

1. **Arduino IDE 2.x**. In *Preferences → Additional boards URLs*, add:
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
   Then install **"Raspberry Pi Pico/RP2040/RP2350"** (Earle Philhower) from the Boards Manager.
2. **Libraries** (Library Manager):
   - **RFM69** by Felix Rusu / LowPowerLab, v1.5 or newer
   - **MIDI Library** by Francois Best (FortySevenEffects)
   - **Adafruit NeoPixel**
   - **Adafruit TinyUSB Library**
3. **Board settings:**

   | Setting | Prop | Receiver |
   |---|---|---|
   | Board | **iLabs Challenger 2040 SubGHz** | **iLabs Challenger 2040 SubGHz** |
   | USB Stack | (default) | **Adafruit TinyUSB** |
   | Flash Size | (default) | **8MB (Sketch: 1MB, FS: 7MB)** |
   | CPU Speed | 50 MHz (saves battery) | (default) |

   **First upload:** hold **BOOT** while plugging in USB, then pick the UF2 port. After that, normal uploads work.

4. **Change the key.** Set `ENCRYPT_KEY` to your own 16-character string in **both** sketches. Change `NETWORK_ID` too if anyone else on site uses RFM69.
5. **Second prop:** flash `prop_tx` again with `TX_NODE_ID 3`. For a different cue, also change `CUE_NUMBER`.

### Sound files

The receiver plays `/cue1.wav` … `/cue4.wav` from its flash. They must be **16-bit PCM WAV**, all at the **same sample rate** (44.1 kHz recommended), mono or stereo. Roughly 40 s of 44.1 kHz stereo or 80 s of mono fits.

Convert with Audacity (*Export → WAV, signed 16-bit*), or with ffmpeg:

```
ffmpeg -i bang.mp3 -ar 44100 -ac 1 -sample_fmt s16 cue1.wav
```

Put the files in a `data/` folder next to `prop_rx.ino`. Then upload them with the **arduino-littlefs-upload** IDE plugin (*Ctrl+Shift+P → "Upload LittleFS to Pico/ESP8266/ESP32"*), with the Serial Monitor closed.

No sound cues wanted? Set `ENABLE_AUDIO false` and the receiver is MIDI-only.

---

## Titan setup

1. Plug in the receiver. Windows should show a MIDI device called **"Prop Trigger"**, with no driver needed.
2. In Titan, open the **trigger / MIDI remote** settings. Create a trigger on the playback or cue list you want, and use **Learn**. Then press the prop button, or the receiver's GO button, with the receiver **armed**.
3. Default mapping: **channel 1**, cue 1 = **note 60 (C4)**, cue 2 = 61, and so on.
4. Set the trigger's **key profile** so a note fires **Go** rather than toggling. Titan's default "Latch" profile toggles on/off as the signal changes. If it still misbehaves, set `SEND_NOTE_OFF false` in `prop_rx.ino`.
5. **One app per MIDI port.** If anything else on that PC needs the note (e.g. a sound cue program), route it through **loopMIDI**.

---

## Bench test

1. Power the receiver over USB. The status LED should go **red** (no prop yet).
2. Switch the prop on. The LED turns **blue** (prop alive, disarmed).
3. Press the prop button. You get a **magenta** flash and nothing fires. The Serial Monitor prints the RSSI.
4. Flip ARM. The LED turns **green**. Press the prop button: you get a **white** flash, the MIDI note, the WAV plays, and the Titan cue goes.
5. Press the receiver's **GO** button. The same cue fires.
6. **Range test in the real room**, with the actor's hand wrapped around the prop, from every position they'll use it. The Serial Monitor shows RSSI once a second:
   - **-50 to -80 dBm:** comfortable.
   - **Worse than -90 dBm:** move the receiver antenna, reroute the prop antenna, or raise `TX_POWER_DBM` up to 14.
7. **Lip-sync check.** If the lamps visibly lag the sound, raise `AUDIO_DELAY_MS` in 10 ms steps.

## Show-day checklist

- [ ] Prop LiPo charged, plus a spare in the corner
- [ ] Receiver LED **blue** when the prop is switched on in the wings
- [ ] **ARM** flipped when the scene starts. Receiver LED **green**
- [ ] Operator knows: no cue within ~½ s of the moment → hit **GO** (receiver) or the cue's GO on Titan
- [ ] **Disarm** after the scene, so the button is safe for the rest of the show

---

## Notes

- **868.3 MHz** is in the 868.0–868.6 MHz licence-exempt sub-band (25 mW ERP, 1% duty cycle). The prop uses 10 dBm and transmits for well under 1% of the time.
- **Never transmit without an antenna attached.** With a U.FL board, power up only with the antenna connected.
- **The receiver never transmits.** This is deliberate: there's no ACK to wait for, and no RF next to its own USB lines. RP2040 USB can get flaky with a transmitting antenna close by.
- **Battery reading** is only an estimate. A LiPo sits around 3.7 V for most of its life, and the receiver warns (amber blink) below 3.5 V.
