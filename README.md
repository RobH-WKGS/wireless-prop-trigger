# Wireless Prop Trigger

A button in a handheld prop fires a lighting cue in Titan and a sound effect together. Two props (used at different moments) share one receiver and fire the same effect.

```
 [Prop 1, node 2] ─┐
 [Prop 2, node 3] ─┴── 868 MHz ──▶ [Receiver at lighting PC]
 Challenger RP2040 SubGHz            Challenger RP2040 SubGHz
 button + LiPo + FPC antenna               ├─ USB MIDI note ──▶ Titan (T3) trigger ─▶ lighting cue
                                           └─ I2S DAC line out ─▶ isolator ─▶ sound desk
```

Every unit uses the same board, the **iLabs Challenger RP2040 SubGHz 868 MHz**. It's a Feather-format board with an RP2040, an RFM69HCW radio, a LiPo charger and a U.FL antenna connector. Any board can be flashed as a prop or as the receiver, so one spare covers everything.

- **Radio:** RFM69 FSK at 868.3 MHz with AES-128 encryption. It's well clear of the 2.4 GHz phone and Wi-Fi traffic.
- **Reliability:** each press sends 4 identical packets. The receiver de-duplicates them, so one lost packet changes nothing.
- **Latency:** press to MIDI and audio is typically under 20 ms. Titan to DMX to fixtures then adds its own delay. `AUDIO_DELAY_MS` lets you hold the sound back to match the lamps if needed.
- **Heartbeat:** the prop reports once a second, with battery voltage. The receiver's **SIGNAL** LED tells you it's alive *before* the scene.
- **Trigger feedback:** the receiver's **TRIGGER** LED lights for 1 second every time the prop is pressed.
- **Rate limit:** a cue fires at most **once per second**. The prop won't send faster than that, and the receiver ignores anything sooner too, including the prop and GO pressed together.
- **Safety:** with the receiver's ARM switch **off, nothing goes to Titan or the sound desk**: not the props, not the GO button, not the test commands. Presses just flicker the TRIGGER LED, so the actor can test a prop in the wings. When armed, the GO button is the operator's backup.

> ⚠️ **Not yet compiled.** I checked the library calls and the board's pin map against their source, but couldn't run the compiler in my environment. Expect possibly a small fix or two on the first compile. Bench-test before rehearsal.

---

## Parts to purchase

UK stockist: The Pi Hut. Prices change, so check at checkout.

### Core (required)

| # | Item | Qty | Notes |
|---|------|-----|-------|
| 1 | **iLabs Challenger RP2040 SubGHz, 868 MHz, U.FL** | **4** | 2 props, 1 receiver, 1 cold spare (can be flashed as either). About £16 each. **Buy the 868 MHz version**, not the 915 MHz one. Loose headers are included. |
| 2 | **868 MHz U.FL flexible PCB (FPC) antenna**, adhesive | 2–3 | One per prop. Flat, sticks inside the prop shell. Must say 868 MHz (or 860–930 MHz). |
| 3 | **U.FL to SMA bulkhead pigtail** (~10–15 cm) | 1 | Receiver: brings the antenna outside the box. |
| 4 | **868 MHz SMA whip / rubber-duck antenna** | 1 | Receiver. Mount it high, with line of sight to the stage. |
| 5 | 3.7 V LiPo, **2.0 mm JST-PH 2-pin**, ~400–500 mAh | 2 | One per prop. About 25 h runtime. **Check polarity against the board's + and − marks**: cells vary between sellers. |
| 6 | Momentary push button for the prop (e.g. 6–12 mm tactile or 7 mm panel push) | 2 | Pick to suit the prop. Normally-open. |
| 7 | Miniature slide switch (SPST) | 2 | Prop power. See wiring notes. |
| 8 | 100 kΩ resistor | 4 | 2 per prop, as a battery-voltage divider. |
| 8a | 100 nF ceramic capacitor | 2 | Optional, one per prop, across R2 to steady the battery reading. |
| 9 | **MCU-5102 / CJMCU-5102 PCM5102A I2S DAC module** | 1 (+1 spare) | Line-level stereo out on its own 3.5 mm jack. Its control pins are wired, see below. |
| 10 | **Line-level isolator / DI** (e.g. Behringer HD400, ART DTI, or a passive DI box) | 1 | Breaks the USB-to-sound-desk ground loop. Don't skip this one. |
| 11 | 3.5 mm stereo to 2× ¼" jack or 2× XLR cable (to suit the isolator) | 1 | Line out to the isolator. |
| 12 | USB-C **data** cable, 1–2 m | 1 | Receiver to lighting PC. A charge-only cable won't enumerate. |

### Receiver panel

| # | Item | Qty | Notes |
|---|------|-----|-------|
| 13 | **Plain 5 mm LEDs**: 2 green, 1 red | 3 | SIGNAL 1 and SIGNAL 2 (green, one per prop) and TRIGGER (red). |
| 13a | 220 Ω resistor | 3 | One per LED. |
| 14 | **3D-printed case** (see [`enclosure/`](enclosure/README.md)) | 1 | 180 × 115 mm sloped desk console, two-colour labels. |
| 15 | Flight / missile toggle switch with flip cover (12 mm bushing) | 1 | ARM. |
| 16 | 30 mm arcade button | 1 | Backup GO. |
| 17 | M3 × 10 socket head screws + M3 heat-set inserts | 4 each | Case fixings. |
| 18 | 3.5 mm stereo **panel-mount extension cable** (plug to panel socket, ~6 mm thread, ~15–20 cm) | 1 | Plugs into the DAC module's jack and brings LINE OUT to the back panel. No soldering on the module. |
| 19 | 12 mm self-adhesive rubber feet | 4 | |

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

![Receiver schematic](docs/prop_rx_schematic.svg)

(PNG version: [`docs/prop_rx_schematic.png`](docs/prop_rx_schematic.png). Regenerate with `python docs/prop_rx_schematic.py docs/prop_rx_schematic.svg`.)

| Board pin | Connects to |
|---|---|
| **D5** | DAC **BCK** |
| **D6** | DAC **LCK** (LRCLK, which the code drives as BCLK+1) |
| **D9** | DAC **DIN** |
| **3V3** | DAC **3.3V** pin. Leave the DAC's **VCC** pin **unconnected** |
| **GND** | DAC **GND**, and both LED **cathodes** (short leg, flat side) |
| **D10** | **220 Ω** → **green SIGNAL 1** (prop 1) LED anode (long leg) |
| **D11** | **220 Ω** → **red TRIGGER** LED anode (long leg) |
| **D12** | **220 Ω** → **green SIGNAL 2** (prop 2) LED anode (long leg) |
| **A0** | ARM switch, other leg to **GND** (closed = armed) |
| **A1** | GO button, other leg to **GND** |
| U.FL | Bulkhead SMA pigtail and 868 MHz antenna |
| DAC 3.5 mm jack | Panel-mount extension cable → LINE OUT on the back panel |

**LED resistors:** 220 Ω gives about 5 mA, plenty for a panel indicator. Brightness is also set in software with `LED_BRIGHTNESS`, so you can dim them for a dark backstage. A bright "pure green" LED can look dim on 3.3 V; drop its resistor to 100 Ω. Keep the red at 220 Ω or more.

**What the LEDs show:**

| LED | Colour | Meaning |
|---|---|---|
| **SIGNAL 1 / SIGNAL 2** (green) | off | That prop not heard for 3 s (off, flat, or out of range) |
| | solid | Prop connected, good signal |
| | slow blink | Prop connected, weak signal (worse than −90 dBm) |
| | fast blink | That prop's battery is low |
| **TRIGGER** (red) | solid for 1 s | Cue fired (either prop, or GO) |
| | flicker for 1 s | A prop or GO pressed while **disarmed**; nothing fired |
| | 2 short blips every 3 s | **No USB host: no MIDI link.** Charge-only cable, PC off, or USB not enumerating |

All three LEDs light for half a second at power-up as a lamp test. All blinking together means the radio failed to start. Whether the receiver is armed shows on the flight switch itself.

**DAC control pins (MCU-5102):** this module brings the PCM5102A's control pins out as header pins instead of solder pads. They must be wired, not left floating:

| DAC pin | Connect to | Why |
|---|---|---|
| **SCL** | GND | System clock; tied low, the chip makes its own from BCK |
| **FLT** | GND | Normal-latency filter |
| **DMP** | GND | De-emphasis off |
| **FMT** | GND | Standard I2S format |
| **XMT** | **3.3V** (the same pin the Challenger's 3V3 feeds) | Un-mutes the output. Left low, the DAC stays silent |

**Why 3V3 and not VCC:** VCC goes through the module's own regulator, which needs headroom above 3.3 V. That only works from USB 5 V, so the DAC goes silent on battery, and on a LiPo the regulator can leave the chip under-powered. Feeding the Challenger's regulated 3V3 straight into the module's 3.3V pin works on USB and on battery alike. The DAC draws only about 20 mA.

**No audio? Test the DAC on its own** (Serial Monitor at any baud rate, receiver on USB):
- Type **`s`** to print the audio status: which `cueN.wav` files were found, and whether I2S is running.
- With **ARM on**, type **`t`** to play a **1 kHz test tone** for 2 seconds. Without a laptop, power up with **ARM on and GO held** for the same tone. Disarmed, the tone is refused so nothing reaches the sound desk.
- Tone but no cue sound → the WAV files aren't loaded or aren't 16-bit PCM. Re-upload the `data` folder.
- No tone either → check the DAC wiring: XMT high, FLT/DMP/SCL/FMT to GND, 3.3 V on the module's 3.3V pin, and BCK/LCK/DIN on D5/D6/D9.

---

## Software setup

1. **Arduino IDE 2.x**. In *Preferences → Additional boards URLs*, add:
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
   Then install **"Raspberry Pi Pico/RP2040/RP2350"** (Earle Philhower) from the Boards Manager.
2. **Libraries** (Library Manager): only **MIDI Library** by Francois Best (FortySevenEffects), for the receiver.
   - The **RFM69** radio library is **bundled** inside each sketch (`src/RFM69`). It's LowPowerLab's v1.6.0 (GPL-3) with a one-line patch so it builds on the RP2040 core. You don't need to install it, and an installed copy won't interfere.
   - **TinyUSB**, I2S and LittleFS come with the RP2040 board package. Don't install Adafruit TinyUSB separately; a second copy can clash with the bundled one.
   - Keep each sketch's `src` folder next to its `.ino` file when you copy or download them.

   Both sketches have been test-compiled with arduino-pico 6.3.2 for the Challenger 2040 SubGHz, with no errors or warnings.
3. **Board settings:**

   | Setting | Prop | Receiver |
   |---|---|---|
   | Board | **iLabs Challenger 2040 SubGHz** | **iLabs Challenger 2040 SubGHz** |
   | USB Stack | (default) | **Adafruit TinyUSB** |
   | Flash Size | (default) | **8MB (Sketch: 6MB, FS: 2MB)** |
   | CPU Speed | 50 MHz (saves battery) | (default) |

   **First upload:** hold **BOOT** while plugging in USB, then pick the UF2 port. After that, normal uploads work.

4. **Change the key.** Set `ENCRYPT_KEY` to your own 16-character string in **both** sketches. Change `NETWORK_ID` too if anyone else on site uses RFM69.
5. **Two props:** flash the first transmitter with `TX_NODE_ID 2` (**PROP 1**, SIGNAL 1) and the second with `TX_NODE_ID 3` (**PROP 2**, SIGNAL 2). Leave `CUE_NUMBER 1` on both so they fire the same effect. If both are pressed within a second, only the first fires. Label the props so they don't get swapped.

### Sound files

The receiver plays `/cue1.wav` … `/cue4.wav` from its flash. They must be **16-bit PCM WAV**, all at the **same sample rate** (44.1 kHz recommended), mono or stereo. A 2 MB filesystem holds roughly 11 s of 44.1 kHz stereo or 23 s of mono. For longer effects pick a bigger FS in **Flash Size**, at the cost of slower data uploads.

Convert with Audacity (*Export → WAV, signed 16-bit*), or with ffmpeg:

```
ffmpeg -i bang.mp3 -ar 44100 -ac 1 -sample_fmt s16 cue1.wav
```

Name them exactly `cue1.wav` to `cue4.wav` (lower case); other names are ignored. Put them in a `data/` folder next to `prop_rx.ino`. Then upload them with the **arduino-littlefs-upload** IDE plugin (*Ctrl+Shift+P → "Upload LittleFS to Pico/ESP8266/ESP32"*), with the Serial Monitor closed.

- **Use the same Flash Size for the sketch upload and the data upload.** The filesystem's position on the flash depends on it. If they differ, or Flash Size is left on the board's default **"8MB (no FS)"**, the receiver finds no files. After changing Flash Size, upload the **sketch and then the data** again.
- **The data upload always writes the whole filesystem**, whatever size your files are. That's why a small file still takes a while; a 2 MB filesystem uploads much faster than 7 MB.
- **Check what's on the board:** open the Serial Monitor and type **`s`**. It shows the filesystem size, every file on the flash, and which `cueN.wav` files it will play.

No sound cues wanted? Set `ENABLE_AUDIO false` and the receiver is MIDI-only.

---

## Titan setup

1. Plug in the receiver. Windows should show a MIDI device called **"Prop Trigger"**, with no driver needed.
2. In Titan, open the **trigger / MIDI remote** settings. Create a trigger on the playback or cue list you want, and use **Learn**. Then press the prop button, or the receiver's GO button, with the receiver **armed**.
3. Default mapping: **channel 1**, cue 1 = **note 60 (C4)**, cue 2 = 61, and so on.
4. Set the trigger's **key profile** so a note fires **Go** rather than toggling. Titan's default "Latch" profile toggles on/off as the signal changes. If it still misbehaves, set `SEND_NOTE_OFF false` in `prop_rx.ino`.
5. **One app per MIDI port.** If anything else on that PC needs the note (e.g. a sound cue program), route it through **loopMIDI**.

### MIDI not reaching Titan?

Open the Arduino Serial Monitor (any baud rate) with the receiver plugged into the lighting PC. Type **`h`** to list the commands.

1. **Is USB connected?** Type **`m`**.
   - "USB: NOT connected" means the PC isn't talking to the receiver. The red TRIGGER LED also gives **two short blips every 3 s**. Usually it's a **charge-only cable**: swap it for a known data cable (one that works for file transfer to a phone). A USB hub or front-panel port can also be at fault, so try a rear port.
   - If the Serial Monitor itself can't see the board, that alone suggests a charge-only cable.
2. **Is the note leaving the receiver?** Every MIDI message is logged, e.g. `MIDI OUT note-on ch 1 note 60 (C4) vel 127 [USB host connected]`. With **ARM on**, type **`n`** to send a **test note** on its own: no audio, and no one-second lockout, so it's ideal for Titan's **Learn**. Disarmed, it's refused.
3. **Is Windows seeing it?** Check that **"Prop Trigger"** appears as a MIDI device (Device Manager → Sound, video and game controllers). A free MIDI monitor such as **MIDI-OX** or **MIDIView** should show the note when you type `n`. **Close the monitor afterwards**, because Windows lets only one program open a MIDI input at a time. Restart Titan if it was open while the monitor had the port.
4. **Is Titan listening?** Titan only picks up MIDI devices present when it starts, so plug the receiver in **before** starting Titan. Check the device is enabled as a MIDI input in Titan's settings, then redo the trigger's Learn using `n` (with ARM on).
5. **Messages *from* the PC** are logged as `MIDI IN …`. If Titan or a MIDI tool sends anything, you'll see it, which proves the link works both ways.
6. **Receiver not listed in USB Expert → Connected Panels?** Titan's USB service hasn't picked it up.
   - With the receiver plugged in, use **Tools → Acw Service** to restart the service (or **Reset** / **Cycle USB Ports**), then restart Titan.
   - Still missing: set `USB_MIDI_ONLY true` in `prop_rx.ino` and upload. The receiver then appears as a plain MIDI-only device with no serial port, which some hardware-focused USB services need. Serial Monitor diagnostics stop in this mode; the LEDs still work. To upload again afterwards, **hold the board's BOOT button while plugging in USB**, because there's no serial port for the IDE to reset.
   - Note that USB Expert's **Watch MIDI** shows the MIDI of the panel selected in Connected Panels. Select the receiver's entry once it appears, not "Mobile DMX".

---

## Bench test

1. Power the receiver over USB. All three LEDs flash on briefly, then the **SIGNAL** LEDs stay off (no props yet). If **TRIGGER** keeps double-blipping, the PC isn't connected over USB (see *MIDI not reaching Titan?*).
2. Switch on prop 1, then prop 2. **SIGNAL 1** and **SIGNAL 2** come on solid. If the wrong one lights, that transmitter has the wrong `TX_NODE_ID`.
3. With ARM off, press each prop's button. **TRIGGER** flickers for 1 s and nothing fires. The Serial Monitor names the prop and prints its RSSI.
4. Flip ARM. Press each prop's button in turn, more than a second apart: **TRIGGER** lights solid for 1 s, the MIDI note goes out, the WAV plays, and the Titan cue goes.
5. Press the prop button twice quickly. Only the first press fires, and the Serial Monitor logs the second as ignored. Wait a second and it fires again.
6. Press the receiver's **GO** button: the same cue fires. Flip ARM off and press GO again: **TRIGGER** flickers, nothing fires, and the Serial Monitor logs `GO ignored (DISARMED)`.
7. **Range test in the real room**, with the actor's hand wrapped around the prop, from every position they'll use it. The Serial Monitor shows both props' RSSI every 2 s:
   - **-50 to -80 dBm:** comfortable.
   - **Worse than -90 dBm:** move the receiver antenna, reroute the prop antenna, or raise `TX_POWER_DBM` up to 14.
8. **Lip-sync check.** If the lamps visibly lag the sound, raise `AUDIO_DELAY_MS` in 10 ms steps.

## Show-day checklist

- [ ] Both prop LiPos charged, plus spares in the corner
- [ ] Receiver plugged into the lighting PC **before Titan starts**. **TRIGGER** not double-blipping (USB OK)
- [ ] **SIGNAL 1** and **SIGNAL 2** **solid** when the props are switched on in the wings (not blinking). A test press on each makes **TRIGGER** flicker
- [ ] **ARM** flipped when the scene starts
- [ ] Operator knows: no cue within ~½ s of the moment → hit **GO** (receiver, only works when armed) or the cue's GO on Titan
- [ ] **Disarm** after the scene, so the button is safe for the rest of the show

---

## Notes

- **868.3 MHz** is in the 868.0–868.6 MHz licence-exempt sub-band (25 mW ERP, 1% duty cycle). The prop uses 10 dBm and transmits for well under 1% of the time.
- **Never transmit without an antenna attached.** With a U.FL board, power up only with the antenna connected.
- **The receiver never transmits.** This is deliberate: there's no ACK to wait for, and no RF next to its own USB lines. RP2040 USB can get flaky with a transmitting antenna close by.
- **Battery reading** is only an estimate. A LiPo sits around 3.7 V for most of its life, and the receiver warns (amber blink) below 3.5 V.
