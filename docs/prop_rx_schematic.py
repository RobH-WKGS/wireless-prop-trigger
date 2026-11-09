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
            elm.IcPin(name='D5 (GPIO2)', side='right', slot='8/8', anchorname='D5'),
            elm.IcPin(name='D6 (GPIO3)', side='right', slot='7/8', anchorname='D6'),
            elm.IcPin(name='D9 (GPIO4)', side='right', slot='6/8', anchorname='D9'),
            elm.IcPin(name='3V3', side='right', slot='5/8', anchorname='V33M'),
            elm.IcPin(name='GND', side='right', slot='4/8', anchorname='GNDR'),
            elm.IcPin(name='D10 (GPIO5)', side='right', slot='3/8', anchorname='D10'),
            elm.IcPin(name='D11 (GPIO6)', side='right', slot='2/8', anchorname='D11'),
            elm.IcPin(name='D12 (GPIO7)', side='right', slot='1/8', anchorname='D12'),
            elm.IcPin(name='U.FL', side='top', slot='2/3', anchorname='ANT'),
            elm.IcPin(name='USB-C', side='top', slot='1/3', anchorname='USBC'),
        ],
        size=(5.5, 14.2), pinspacing=1.6, edgepadW=0.9, leadlen=1.0,
    ).label('Challenger RP2040\nSubGHz 868 MHz\n(RFM69HCW on SPI1)', loc='center', ofst=(-0.9, -1.6), fontsize=12)
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


    # ---------------- right: MCU-5102 (PCM5102A) DAC module ----------------
    dac = elm.Ic(
        pins=[
            elm.IcPin(name='BCK', side='left', slot='6/6', anchorname='BCK'),
            elm.IcPin(name='LCK', side='left', slot='5/6', anchorname='LCK'),
            elm.IcPin(name='DIN', side='left', slot='4/6', anchorname='DIN'),
            elm.IcPin(name='3.3V', side='left', slot='3/6', anchorname='V33'),
            elm.IcPin(name='GND', side='left', slot='2/6', anchorname='GND'),
            elm.IcPin(name='VCC', side='right', slot='6/6', anchorname='VCC'),
            elm.IcPin(name='XMT', side='right', slot='5/6', anchorname='XMT'),
            elm.IcPin(name='FLT', side='right', slot='4/6', anchorname='FLT'),
            elm.IcPin(name='DMP', side='right', slot='3/6', anchorname='DMP'),
            elm.IcPin(name='SCL', side='right', slot='2/6', anchorname='SCL'),
            elm.IcPin(name='FMT', side='right', slot='1/6', anchorname='FMT'),
            elm.IcPin(name='3.5 mm jack', side='bot', slot='1/1', anchorname='JACK'),
        ],
        size=(4.6, 10.4), pinspacing=1.6, edgepadW=0.6, leadlen=1.0,
    ).label('MCU-5102\n(PCM5102A)\nI2S DAC\nmodule', loc='center', fontsize=12)
    # place DAC so BCK lines up with D5
    d += dac.theta(0).at((mcu.D5[0] + 6.5, mcu.D5[1])).anchor('BCK')

    d += elm.Line().at(mcu.D5).to(dac.BCK)
    d += elm.Line().at(mcu.D6).to(dac.LCK)
    d += elm.Line().at(mcu.D9).to(dac.DIN)
    d += elm.Line().at(mcu.V33M).to(dac.V33)
    d += elm.Line().at(mcu.GNDR).to(dac.GND)

    # VCC left unconnected: the module is fed straight onto its 3.3 V rail
    d += elm.Line().at(dac.VCC).right(0.7)
    d += elm.Label().label('n/c (leave unconnected)', loc='right', halign='left', ofst=(0.2, 0), fontsize=10)
    # XMT tied to 3.3 V (un-mute)
    d += elm.Line().at(dac.XMT).right(0.7)
    d += elm.Label().label('to 3.3V pin (un-mute)', loc='right', halign='left', ofst=(0.2, 0), fontsize=10)
    # FLT, DMP, SCL, FMT tied to GND
    xgnd = dac.FLT[0] + 0.9
    for pin in (dac.FLT, dac.DMP, dac.SCL, dac.FMT):
        d += elm.Line().at(pin).tox(xgnd)
        d += elm.Dot().at((xgnd, pin[1]))
    d += elm.Line().at((xgnd, dac.FLT[1])).toy(dac.FMT[1] - 0.8)
    d += elm.Ground()
    d += elm.Label().at((xgnd + 0.2, (dac.FLT[1] + dac.FMT[1]) / 2)).label('FLT, DMP, SCL, FMT\nto GND', loc='right', halign='left', ofst=(0.3, 0), fontsize=10)

    # line out: the module's own jack -> panel-mount extension cable
    d += elm.Line().at(dac.JACK).down(1.0)
    d += elm.Label().label('3.5 mm panel-mount extension cable\n→ LINE OUT on back panel\n→ line isolator / DI → sound desk', loc='bottom', fontsize=10)

    # ---------------- LEDs: plain 5 mm, one pin each ----------------
    xg = dac.GND[0] + 3.2           # shared LED ground column
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
    y1 = mcu.D12[1] - 4.0
    y2 = y1 - 2.6
    y3 = y2 - 2.6
    led_chain(mcu.D10, y1, 2.0, 'R3 220 Ω', 'SIGNAL 1 (prop 1)\n5 mm green')
    led_chain(mcu.D11, y2, 1.2, 'R4 220 Ω', 'TRIGGER\n5 mm red')
    led_chain(mcu.D12, y3, 0.4, 'R5 220 Ω', 'SIGNAL 2 (prop 2)\n5 mm green')
    d += elm.Line().at((xg, y1)).toy(y3 - 1.0)
    d += elm.Ground()
    d += elm.Label().at((xg + 1.0, y2)).label('cathodes (short leg,\nflat side) to GND', loc='right', halign='left', ofst=(0.3, 0), fontsize=10)

    # ---------------- optional LiPo ----------------
    d += elm.Label().at((mcu.GNDL[0] + 3.5, mcu.D12[1] - 3.4)).label(
        'Optional LiPo on the board\'s JST socket\n(charges from USB-C)', loc='center', fontsize=10)

    # ---------------- notes ----------------
    notes = (
        'Notes\n'
        '• Powered from the lighting PC over USB-C, which is also the USB MIDI link.\n'
        '• A0 / A1 use internal pull-ups: switch and button pull them to GND.\n'
        '• I2S: D5 = BCK, D6 = LRCK (always BCK + 1), D9 = DIN.\n'
        '• DAC powered from the Challenger 3V3 into its 3.3V pin; VCC left unconnected.\n'
        '   Works on USB and on battery (the module\'s own regulator needs too much headroom on a LiPo).\n'
        '   FLT, DMP, SCL and FMT are header pins on this module and must be wired to GND.\n'
        '• Two props: transmitters flashed as node 2 (prop 1) and node 3 (prop 2).\n'
        '• LEDs: anode (long leg) to the resistor, cathode to GND. 220 Ω is about 5 mA.\n'
        '   A dim bright-green LED can go down to 100 Ω (keep red at 220 Ω or more).\n'
        '   Brightness is also set in software (LED_BRIGHTNESS).\n'
        '• LINE OUT feeds the sound desk through an isolator to avoid a ground-loop hum.\n'
        '• Radio is internal: leave GPIO 13, 14, 15, 18 unused.\n'
        '• Never power up without the antenna fitted.'
    )
    d += elm.Label().at((xl - 1.4, y3 - 3.0)).label(notes, loc='right', halign='left', valign='top', fontsize=10)

    d.save(out)
print('saved', out)
