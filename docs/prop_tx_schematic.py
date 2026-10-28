import sys
import schemdraw
import schemdraw.elements as elm

out = sys.argv[1]
schemdraw.config(fontsize=12, lw=1.4)

with schemdraw.Drawing(show=False) as d:
    d.config(unit=2.5)

    ic = elm.Ic(
        pins=[
            elm.IcPin(name='BAT', side='left', slot='3/3', anchorname='BAT'),
            elm.IcPin(name='A2', side='left', slot='2/3', anchorname='A2'),
            elm.IcPin(name='GND', side='left', slot='1/3', anchorname='GNDL'),
            elm.IcPin(name='D5 (GPIO2)', side='right', slot='3/3', anchorname='D5'),
            elm.IcPin(name='EN', side='right', slot='2/3', anchorname='EN'),
            elm.IcPin(name='GND', side='right', slot='1/3', anchorname='GNDR'),
            elm.IcPin(name='U.FL', side='top', slot='1/1', anchorname='ANT'),
        ],
        size=(5.2, 6.5), pinspacing=1.8, edgepadW=0.9, leadlen=1.2,
    ).label('Challenger RP2040\nSubGHz 868 MHz\n(RFM69HCW on SPI1)', loc='center', fontsize=12)
    d += ic

    # --- Antenna on U.FL ---------------------------------------------------
    d += elm.Line().at(ic.ANT).up(0.6)
    d += elm.Antenna().label('868 MHz FPC antenna\n(U.FL, stuck inside shell,\naway from fingers)', loc='right', ofst=(0.3, -0.6), fontsize=10)

    # --- Left: smoothing cap, divider -> A2, LiPo on JST ---------------------
    X0 = ic.BAT[0]
    xc, xr, xb = X0 - 2.0, X0 - 4.4, X0 - 8.0       # cap, divider, battery columns
    yb, ya, yg = ic.BAT[1], ic.A2[1], ic.GNDL[1]

    d += elm.Line().at(ic.BAT).tox(xb)               # BAT rail
    d += elm.Line().at(ic.GNDL).tox(xb)              # GND rail
    d += elm.Line().at(ic.A2).tox(xr)                # A2 tap

    d += elm.Dot().at((xr, yb))
    d += elm.Resistor().endpoints((xr, yb), (xr, ya)).label('R1\n100 kΩ', loc='bottom', ofst=0.15)
    d += elm.Dot().at((xr, ya))
    d += elm.Resistor().endpoints((xr, ya), (xr, yg)).label('R2\n100 kΩ', loc='bottom', ofst=0.15)
    d += elm.Dot().at((xr, yg))

    d += elm.Dot().at((xc, ya))
    d += elm.Capacitor().endpoints((xc, ya), (xc, yg)).label('C1\n100 nF\n(optional)', loc='bottom', ofst=0.15)
    d += elm.Dot().at((xc, yg))

    d += elm.Battery().endpoints((xb, yg), (xb, yb)).reverse().label(
        'LiPo 3.7 V\n400–500 mAh\n2.0 mm JST-PH', loc='top', ofst=0.3)
    d += elm.Label().at((xb, yb)).label('+', loc='left', ofst=(0, 0.3))
    d += elm.Ground().at((xb, yg))
    lipo_bot = (xb, yg)

    # --- Right: button and power switch ------------------------------------------
    d += (d5_line := elm.Line().at(ic.D5).right(1.2))
    d += elm.Button().right().label('Trigger button\n(momentary, N/O)', loc='top')
    btn_end = d.here
    d += elm.Line().at(ic.EN).right(1.2)
    d += elm.Switch().right().label('Power switch\n(closed = OFF)', loc='top')
    sw_end = d.here
    d += elm.Line().at(ic.GNDR).right(1.2 + 3.0)
    gnd_r_end = d.here
    d += elm.Line().at(btn_end).right(1.0)
    rail_top = d.here
    d += elm.Line().down().toy(gnd_r_end)
    rail_bot = d.here
    d += elm.Line().at(sw_end).right(1.0)
    d += elm.Dot()
    d += elm.Line().at(gnd_r_end).tox(rail_bot)
    d += elm.Dot()
    d += elm.Ground().at(rail_bot)

    # --- Notes ---------------------------------------------------------------------
    notes = (
        'Notes\n'
        '• LiPo plugs into the JST socket; the BAT pin is the same net (tap R1 there).\n'
        '• D5 uses the internal pull-up: button pulls it to GND.\n'
        '• R1/R2 halve the battery voltage for A2 (code ×2). Draws ~20 µA.\n'
        '   Skip them by setting BATT_PIN to -1 in prop_tx.ino.\n'
        '• EN to GND turns off the 3.3 V regulator; charging over USB still works.\n'
        '   No EN pin on your board? Put the switch in the LiPo + lead instead.\n'
        '• Radio is internal: leave GPIO 13, 14, 15, 18 unused.\n'
        '• Never power up without the antenna fitted.'
    )
    d += elm.Label().at((ic.GNDL[0] - 11.5, ic.GNDL[1] - 3.6)).label(notes, loc='right', halign='left', fontsize=10)

    d.save(out)
print('saved', out)
