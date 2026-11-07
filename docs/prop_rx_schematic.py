"""Receiver schematic. usage: python prop_rx_schematic.py out.svg   (pip install schemdraw)"""
import sys
import schemdraw
import schemdraw.elements as elm

out = sys.argv[1]
schemdraw.config(fontsize=12, lw=1.4)

with schemdraw.Drawing(show=False) as d:
    d.config(unit=2.5)

    # ---------------- Challenger RP2040 SubGHz ----------------
    mcu = elm.Ic(
        pins=[
            elm.IcPin(name='A0', side='left', slot='3/4', anchorname='A0'),
            elm.IcPin(name='A1', side='left', slot='2/4', anchorname='A1'),
            elm.IcPin(name='GND', side='left', slot='1/4', anchorname='GNDL'),
            elm.IcPin(name='D5 (GPIO2)', side='right', slot='7/7', anchorname='D5'),
            elm.IcPin(name='D6 (GPIO3)', side='right', slot='6/7', anchorname='D6'),
            elm.IcPin(name='D9 (GPIO4)', side='right', slot='5/7', anchorname='D9'),
            elm.IcPin(name='3V3', side='right', slot='4/7', anchorname='V33R'),
            elm.IcPin(name='GND', side='right', slot='3/7', anchorname='GNDR'),
            elm.IcPin(name='D10 (GPIO5)', side='right', slot='2/7', anchorname='D10'),
            elm.IcPin(name='D11 (GPIO6)', side='right', slot='1/7', anchorname='D11'),
            elm.IcPin(name='U.FL', side='top', slot='2/3', anchorname='ANT'),
            elm.IcPin(name='USB-C', side='top', slot='1/3', anchorname='USBC'),
        ],
        size=(5.5, 12.6), pinspacing=1.6, edgepadW=0.9, leadlen=1.0,
    ).label('Challenger RP2040\nSubGHz 868 MHz\n(RFM69HCW on SPI1)', loc='center', fontsize=12)
    d += mcu

    # ---------------- top: USB-C to PC, antenna ----------------
    d += elm.Line().at(mcu.USBC).up(1.2)
    d += elm.Label().label('USB-C cable → lighting PC\n(power + USB MIDI)', loc='top', fontsize=11)
    d += elm.Line().at(mcu.ANT).up(0.6)
    d += elm.Antenna().label('SMA bulkhead\n+ 868 MHz antenna\n(U.FL pigtail)', loc='right', ofst=(0.3, -0.9), fontsize=10)

    # ---------------- left: ARM switch, GO button ----------------
    xl = mcu.A0[0] - 8.5
    d += elm.Line().at(mcu.A0).left(1.2)
    d += elm.Switch().left().label('ARM\nflight toggle\n(closed = armed)', loc='top', fontsize=11)
    arm_end = d.here
    d += elm.Line().at(mcu.A1).left(1.2)
    d += elm.Button().left().label('GO\narcade button', loc='bottom', fontsize=11)
    go_end = d.here
    d += elm.Line().at(arm_end).tox(xl)
    d += elm.Line().at(go_end).tox(xl)
    d += elm.Dot().at((xl, go_end[1]))
    d += elm.Line().at((xl, arm_end[1])).toy(mcu.GNDL[1])
    d += elm.Line().at(mcu.GNDL).tox(xl)
    d += elm.Dot().at((xl, mcu.GNDL[1]))
    d += elm.Ground().at((xl, mcu.GNDL[1]))


    # ---------------- right: PCM5102A DAC ----------------
    dac = elm.Ic(
        pins=[
            elm.IcPin(name='BCK', side='left', slot='6/6', anchorname='BCK'),
            elm.IcPin(name='LCK', side='left', slot='5/6', anchorname='LCK'),
            elm.IcPin(name='DIN', side='left', slot='4/6', anchorname='DIN'),
            elm.IcPin(name='VIN', side='left', slot='3/6', anchorname='VIN'),
            elm.IcPin(name='GND', side='left', slot='2/6', anchorname='GND'),
            elm.IcPin(name='SCK', side='left', slot='1/6', anchorname='SCK'),
            elm.IcPin(name='LOUT', side='right', slot='3/3', anchorname='LOUT'),
            elm.IcPin(name='ROUT', side='right', slot='2/3', anchorname='ROUT'),
            elm.IcPin(name='AGND', side='right', slot='1/3', anchorname='AGND'),
        ],
        size=(4.2, 10.4), pinspacing=1.6, edgepadW=0.6, leadlen=1.0,
    ).label('PCM5102A\nI2S DAC\nmodule', loc='center', fontsize=12)
    # place DAC so BCK lines up with D5
    d += dac.theta(0).at((mcu.D5[0] + 6.5, mcu.D5[1])).anchor('BCK')

    d += elm.Line().at(mcu.D5).to(dac.BCK)
    d += elm.Line().at(mcu.D6).to(dac.LCK)
    d += elm.Line().at(mcu.D9).to(dac.DIN)
    d += elm.Line().at(mcu.V33R).to(dac.VIN)
    d += elm.Line().at(mcu.GNDR).to(dac.GND)
    # SCK tied to GND
    xs = dac.SCK[0] - 1.2
    d += elm.Line().at(dac.SCK).tox(xs)
    d += elm.Line().toy(dac.GND[1])
    d += elm.Dot()
    d += elm.Label().at((xs, dac.SCK[1])).label('SCK → GND', loc='bottom', ofst=(0, 0.1), fontsize=10)

    # line-out jack
    xj = dac.LOUT[0] + 2.4
    jack_box = elm.Ic(
        pins=[elm.IcPin(name='Tip (L)', side='left', slot='3/3', anchorname='T'),
              elm.IcPin(name='Ring (R)', side='left', slot='2/3', anchorname='R'),
              elm.IcPin(name='Sleeve', side='left', slot='1/3', anchorname='S')],
        size=(5.2, 4.8), pinspacing=1.6, edgepadW=0.2, leadlen=0.6,
    ).label('3.5 mm\npanel jack\nLINE OUT', loc='center', ofst=(0.9, 0), fontsize=10)
    d += jack_box.theta(0).at((xj, dac.LOUT[1])).anchor('T')
    d += elm.Line().at(dac.LOUT).to(jack_box.T)
    d += elm.Line().at(dac.ROUT).to(jack_box.R)
    d += elm.Line().at(dac.AGND).to(jack_box.S)
    d += elm.Label().at((jack_box.T[0] + 3.2, jack_box.S[1] - 1.6)).label(
        'to line isolator / DI → sound desk', loc='bottom', fontsize=10)

    # ---------------- LEDs: plain 5 mm, one pin each ----------------
    xg = dac.SCK[0] + 3.2           # shared LED ground column
    def led_chain(pin, y, col, rname, lname):
        # each pin gets its own vertical run (col) so the two never touch
        x0 = mcu.D10[0] + col
        xr = mcu.D10[0] + 2.8
        d.add(elm.Line().at(pin).tox(x0))
        d.add(elm.Line().at((x0, pin[1])).toy(y))
        d.add(elm.Line().at((x0, y)).tox(xr))
        d.add(elm.Resistor().at((xr, y)).right().label(rname, loc='top', fontsize=10))
        d.add(elm.LED().right().label(lname, loc='top', fontsize=10))
        d.add(elm.Line().tox(xg))
        d.add(elm.Dot())
    y1 = mcu.D11[1] - 4.0
    y2 = y1 - 2.6
    led_chain(mcu.D10, y1, 2.0, 'R3 220 Ω', 'LED 1 SIGNAL\n5 mm green')
    led_chain(mcu.D11, y2, 1.0, 'R4 220 Ω', 'LED 2 TRIGGER\n5 mm red')
    d += elm.Line().at((xg, y1)).toy(y2 - 1.0)
    d += elm.Ground()
    d += elm.Label().at((xg + 1.0, (y1 + y2) / 2)).label('cathodes (short leg,\nflat side) to GND', loc='right', fontsize=10)

    # ---------------- optional LiPo ----------------
    d += elm.Label().at((mcu.GNDL[0] + 3.5, mcu.D11[1] - 2.4)).label(
        'Optional LiPo on the board\'s JST socket\n(charges from USB-C)', loc='center', fontsize=10)

    # ---------------- notes ----------------
    notes = (
        'Notes\n'
        '• Powered from the lighting PC over USB-C, which is also the USB MIDI link.\n'
        '• A0 / A1 use internal pull-ups: switch and button pull them to GND.\n'
        '• I2S: D5 = BCK, D6 = LRCK (always BCK + 1), D9 = DIN.\n'
        '• DAC solder pads: SCK to GND, FLT low, DEMP low, XSMT high, FMT low.\n'
        '• LEDs: anode (long leg) to the resistor, cathode to GND. 220 Ω is about 5 mA.\n'
        '   A dim bright-green LED can go down to 100 Ω (keep red at 220 Ω or more).\n'
        '   Brightness is also set in software (LED_BRIGHTNESS).\n'
        '• LINE OUT feeds the sound desk through an isolator to avoid a ground-loop hum.\n'
        '• Radio is internal: leave GPIO 13, 14, 15, 18 unused.\n'
        '• Never power up without the antenna fitted.'
    )
    d += elm.Label().at((xl - 1.4, y2 - 3.0)).label(notes, loc='right', halign='left', valign='top', fontsize=10)

    d.save(out)
print('saved', out)
