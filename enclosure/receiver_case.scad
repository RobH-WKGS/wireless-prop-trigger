// =====================================================================
//  Wireless Prop Trigger — RECEIVER CASE
// ---------------------------------------------------------------------
//  Two-part desk console: a base tray (electronics) and a flat control
//  lid (ARM flight switch, GO arcade button, status LED).
//  Held together with 4 x M3 x 10 socket-head cap screws, from the top,
//  into M3 heat-set inserts in the base corner posts.
//
//  Parts — set `part` below, or from the command line:
//    openscad -D 'part="base"' -o base.stl receiver_case.scad
//    "base"        base tray, print floor-down
//    "lid"         control lid, printed TOP-FACE DOWN (already flipped)
//    "fit_panel"   small coupon of the lid holes: check switch/button/LED fit
//    "fit_cradle"  corner of the base around the board: check USB-C + snap fit
//    "assembly"    preview with dummy parts (not for printing)
//
//  Everything is in mm. Measure YOUR switch and button and adjust the
//  "Controls" section before printing the full lid; print the two fit_
//  coupons first (about 20 minutes each).
//
//  Label font: Lato Black (WKGS brand). If Lato isn't installed OpenSCAD
//  falls back to a default font — set label_font to e.g.
//  "Liberation Sans:style=Bold" to silence the warning.
// =====================================================================

part = "assembly";   // [base, lid, fit_panel, fit_cradle, assembly]

/* [Case] */
case_w      = 180;   // X, left-right as the operator sees it
case_d      = 115;   // Y, front (0) to back
base_h      = 52;    // base tray height, floor to rim
lid_t       = 4;     // lid plate thickness
wall        = 3;
floor_t     = 3;
corner_r    = 6;     // outer corner radius
top_chamfer = 1.5;   // chamfer on the lid's top edge
fit_gap     = 0.3;   // clearance between lid skirt/bosses and base walls

/* [Fasteners: M3 socket head] */
fastener      = "insert"; // [insert, tap]  heat-set insert, or tap straight into plastic
insert_d      = 4.0;      // hole for M3 heat-set insert (check your insert's spec)
insert_depth  = 6.5;
tap_d         = 2.6;      // hole for screwing M3 directly into the plastic
screw_clear_d = 3.4;
head_d        = 6.2;      // counterbore for M3 SHCS head (5.5 mm head)
head_depth    = 3.4;      // heads sit flush
post_size     = 11;       // square corner posts in the base
boss_h        = 4;        // lid boss height below the plate (meets the post)

/* [Controls] */
// ARM — flight / missile toggle switch with flip cover
arm_x          = 45;
arm_y          = 48;
arm_hole_d     = 12.2;   // typical missile switch: 12 mm bushing
arm_key_w      = 0;      // anti-rotation key width (0 = none); check your switch
arm_key_l      = 1.5;    // how far the key notch extends past the hole
arm_body_d     = 24;     // clearance envelope under the lid (preview / checks)
arm_body_depth = 38;

// GO — arcade button
go_x           = 135;
go_y           = 52;
go_hole_d      = 30.2;   // 30 mm arcade button (24 mm: 24.2, 60 mm dome: check yours)
go_panel_t     = 2.5;    // lid is thinned to this around the button (snap-in buttons need <= ~3)
go_relief_d    = 46;     // diameter of the thinned area (room for the nut/clips)
go_body_d      = 33;     // clearance envelope under the lid
go_body_depth  = 42;

// STATUS — 5 mm through-hole addressable LED (WS2812/PL9823 5 mm)
led_x          = 90;
led_y          = 92;
led_hole_d     = 5.3;
led_rim_d      = 6.2;    // the LED's flange sits in this, against the lid underside
led_tube_od    = 9.5;
led_tube_h     = 9;

/* [Board: Challenger RP2040 SubGHz, Feather format] */
pcb_l        = 50.8;
pcb_w        = 22.86;
pcb_t        = 1.6;
pcb_clear    = 0.25;   // clearance around the board outline
board_x      = 20;     // left edge of the board (inside the case)
standoff_h   = 3;      // board underside above the floor
usb_w        = 8.94;   // USB-C receptacle
usb_h        = 3.26;
usb_z_above_pcb = 1.63;  // centre of receptacle above PCB top (top-mount = usb_h/2)
usb_hole_w   = 9.8;    // through the thinned wall
usb_hole_h   = 4.2;
usb_recess_w = 15;     // outside recess for the cable's overmould
usb_recess_h = 9.5;
usb_wall_t   = 1.2;    // wall thickness left at the USB port

/* [Back panel] */
sma_x        = 68;     // SMA bulkhead (antenna)
sma_z        = 32;
sma_hole_d   = 6.6;
sma_flat     = 5.9;    // across-flats for D-shaped bulkheads (set = sma_hole_d for round)
jack_x       = 112;    // 3.5 mm panel-mount line-out jack
jack_z       = 30;
jack_hole_d  = 6.2;

/* [Inside] */
battery_bay       = true;
bay_x             = 66;   // left edge of bay (inside)
bay_y             = 10;
bay_w             = 50;   // fits up to ~45 x 32 x 10 mm cells
bay_d             = 36;
bay_wall_h        = 7;
dac_x             = 96;   // PCM5102A module area (left edge)
dac_y             = 72;
dac_w             = 34;
dac_d             = 26;

/* [Labels] */
label_font  = "Lato:style=Black";
label_depth = 0.8;
title       = "WIRELESS PROP TRIGGER";

$fn = 64;
eps = 0.01;

// ---------------------------------------------------------------------
//  Derived values
// ---------------------------------------------------------------------
pcb_y1   = case_d - wall - 0.2;          // USB end of the board (against back wall)
pcb_y0   = pcb_y1 - pcb_l;               // far end of the board
pcb_z0   = floor_t + standoff_h;         // PCB underside
pcb_ztop = pcb_z0 + pcb_t;
usb_x    = board_x + pcb_w / 2;
usb_z    = pcb_ztop + usb_z_above_pcb;
inner_r  = max(corner_r - wall, 0.5);
post_top = base_h - boss_h - 0.2;        // small gap so the lid seats on the rim
post_c   = wall + post_size / 2;         // post centre inset from the outer corner
post_pos = [[post_c, post_c], [case_w - post_c, post_c],
            [post_c, case_d - post_c], [case_w - post_c, case_d - post_c]];

// ---------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------
module rrect(w, d, h, r) {
    // rounded rectangle prism, corner at origin
    r2 = max(r, 0.01);
    hull() for (x = [r2, w - r2], y = [r2, d - r2])
        translate([x, y, 0]) cylinder(r = r2, h = h);
}

module rrect_c(w, h, d, r) {
    // rounded rectangle in the XZ plane, centred on X and Z, extruded along +Y by d
    rotate([-90, 0, 0]) linear_extrude(d)
        offset(r = r) offset(delta = -r) square([w, h], center = true);
}

module label(txt, size, halign = "center", valign = "center") {
    text(txt, size = size, font = label_font, halign = halign, valign = valign);
}

// ---------------------------------------------------------------------
//  BASE
// ---------------------------------------------------------------------
module base_shell() {
    difference() {
        rrect(case_w, case_d, base_h, corner_r);
        translate([wall, wall, floor_t])
            rrect(case_w - 2 * wall, case_d - 2 * wall, base_h, inner_r);
    }
}

module corner_posts() {
    for (p = post_pos) {
        // post fused into the corner walls
        cx = p[0] < case_w / 2 ? wall : case_w - wall - post_size;
        cy = p[1] < case_d / 2 ? wall : case_d - wall - post_size;
        translate([cx, cy, 0]) cube([post_size, post_size, post_top]);
    }
}

module post_holes() {
    for (p = post_pos) translate([p[0], p[1], 0]) {
        if (fastener == "insert") {
            translate([0, 0, post_top - insert_depth]) cylinder(d = insert_d, h = insert_depth + 1);
            translate([0, 0, post_top - 0.6]) cylinder(d1 = insert_d, d2 = insert_d + 1.2, h = 0.61); // lead-in
        } else {
            translate([0, 0, post_top - 10]) cylinder(d = tap_d, h = 11);
        }
    }
}

module board_cradle() {
    x0 = board_x - pcb_clear;
    x1 = board_x + pcb_w + pcb_clear;
    // support strips under each end of the board (clear of the header pads)
    for (y = [pcb_y0 + 0.5, pcb_y1 - 3.0])
        translate([board_x + 1, y, floor_t - eps]) cube([pcb_w - 2, 2.5, standoff_h + eps]);
    // snap posts at the four corners, outside the long edges
    post_t = 1.8;
    lip    = 0.8;
    lip_h  = 1.4;
    for (y = [pcb_y0 + 0.8, pcb_y1 - 4.8], side = [0, 1]) {
        px = side == 0 ? x0 - post_t : x1;
        translate([px, y, floor_t - eps]) {
            cube([post_t, 4, pcb_ztop - floor_t + 0.15 + lip_h]);
            // inward lip with a 45 degree lead-in on top
            lx = side == 0 ? post_t : -lip;
            translate([lx, 0, pcb_ztop - floor_t + 0.15])
                hull() {
                    translate([side == 0 ? 0 : lip - eps, 0, 0]) cube([eps, 4, lip_h]);
                    translate([0, 0, 0]) cube([lip, 4, 0.4]);
                }
        }
    }
    // end stop at the far end: takes the push when a USB plug goes in
    translate([usb_x - 6, pcb_y0 - pcb_clear - 2.5, floor_t - eps])
        cube([12, 2.5, pcb_ztop - floor_t]);
}

module tie_loop(along_x = false) {
    // a small arch for a zip tie / hook-and-loop strap, sitting on the floor
    rotate([0, 0, along_x ? 90 : 0])
    difference() {
        translate([-3.5, -2, 0]) cube([7, 4, 4.5]);
        translate([-2.2, -2 - eps, -eps]) cube([4.4, 4 + 2 * eps, 2.4]);
    }
}

module battery_bay_walls() {
    if (battery_bay) translate([bay_x, bay_y, floor_t - eps]) {
        difference() {
            rrect(bay_w + 3.2, bay_d + 3.2, bay_wall_h, 2);
            translate([1.6, 1.6, -1]) rrect(bay_w, bay_d, bay_wall_h + 2, 1);
            // lead exit toward the board
            translate([4, bay_d, 2]) cube([10, 5, bay_wall_h]);
        }
        // strap loops inside the bay
        for (x = [1.6 + 5, 1.6 + bay_w - 5])
            translate([x, 1.6 + bay_d / 2, 0]) tie_loop(along_x = true);
    }
}

module dac_mount() {
    // two zip-tie loops either side of the DAC module; use foam tape under it
    for (x = [dac_x - 4.5, dac_x + dac_w + 4.5])
        translate([x, dac_y + dac_d / 2, floor_t - eps]) tie_loop(along_x = false);
}

module back_panel_cuts() {
    // USB-C: outside recess for the overmould, then the port opening
    translate([usb_x, case_d - wall + usb_wall_t, usb_z])
        rrect_c(usb_recess_w, usb_recess_h, wall, 2);
    translate([usb_x, case_d - wall - 1, usb_z])
        rrect_c(usb_hole_w, usb_hole_h, wall + 2, 1.2);
    // SMA bulkhead (D-shaped if sma_flat < sma_hole_d)
    translate([sma_x, case_d - wall - 1, sma_z]) rotate([-90, 0, 0])
        linear_extrude(wall + 2) intersection() {
            circle(d = sma_hole_d);
            square([sma_hole_d, sma_flat], center = true);
        }
    // line-out jack
    translate([jack_x, case_d - wall - 1, jack_z]) rotate([-90, 0, 0])
        cylinder(d = jack_hole_d, h = wall + 2);
    // back-panel labels (read correctly from behind)
    for (l = [[usb_x, "USB-C"], [sma_x, "ANT"], [jack_x, "LINE OUT"]])
        translate([l[0], case_d - label_depth, l[1] == "USB-C" ? usb_z + usb_recess_h / 2 + 5 : sma_z - 10])
            rotate([90, 0, 180]) translate([0, 0, -1])
                linear_extrude(label_depth + 1) label(l[1], 4.5);
}

module front_label() {
    translate([case_w / 2, label_depth, base_h / 2])
        rotate([90, 0, 0]) translate([0, 0, -1])
            linear_extrude(label_depth + 1) label(title, 7);
}

module feet_recesses() {
    for (x = [16, case_w - 16], y = [16, case_d - 16])
        translate([x, y, -eps]) cylinder(d = 12.5, h = 1 + eps);  // 12 mm self-adhesive bumpers
}

module base() {
    difference() {
        union() {
            base_shell();
            corner_posts();
            board_cradle();
            battery_bay_walls();
            dac_mount();
        }
        post_holes();
        back_panel_cuts();
        front_label();
        feet_recesses();
    }
}

// ---------------------------------------------------------------------
//  LID  (modelled in place: plate from z = 0 to lid_t, skirt/bosses below)
// ---------------------------------------------------------------------
module lid_plate() {
    hull() {
        rrect(case_w, case_d, lid_t - top_chamfer, corner_r);
        translate([top_chamfer, top_chamfer, 0])
            rrect(case_w - 2 * top_chamfer, case_d - 2 * top_chamfer, lid_t, corner_r - top_chamfer);
    }
}

module lid_underside() {
    iw = case_w - 2 * wall - 2 * fit_gap;
    id = case_d - 2 * wall - 2 * fit_gap;
    // locating skirt just inside the base walls
    translate([wall + fit_gap, wall + fit_gap, -3]) difference() {
        rrect(iw, id, 3 + eps, max(inner_r - fit_gap, 0.5));
        translate([1.6, 1.6, -1]) rrect(iw - 3.2, id - 3.2, 5, max(inner_r - fit_gap - 1.6, 0.5));
    }
    // corner bosses that land on the base posts (trimmed to the rounded inner corners)
    intersection() {
        union() for (p = post_pos) {
            cx = p[0] < case_w / 2 ? wall + fit_gap : case_w - wall - fit_gap - (post_size - fit_gap);
            cy = p[1] < case_d / 2 ? wall + fit_gap : case_d - wall - fit_gap - (post_size - fit_gap);
            translate([cx, cy, -boss_h]) cube([post_size - fit_gap, post_size - fit_gap, boss_h + eps]);
        }
        translate([wall + fit_gap, wall + fit_gap, -boss_h - 1])
            rrect(iw, id, boss_h + 2, max(inner_r - fit_gap, 0.5));
    }
    // LED holder tube
    translate([led_x, led_y, -led_tube_h]) difference() {
        cylinder(d = led_tube_od, h = led_tube_h + eps);
        translate([0, 0, -1]) cylinder(d = led_rim_d, h = led_tube_h + 2);
    }
}

module lid_cuts() {
    // screw holes with counterbores
    for (p = post_pos) translate([p[0], p[1], 0]) {
        translate([0, 0, -boss_h - 1]) cylinder(d = screw_clear_d, h = boss_h + lid_t + 2);
        translate([0, 0, lid_t - head_depth]) cylinder(d = head_d, h = head_depth + 1);
    }
    // ARM switch hole (+ optional key notch)
    translate([arm_x, arm_y, -1]) {
        cylinder(d = arm_hole_d, h = lid_t + 2);
        if (arm_key_w > 0)
            translate([-arm_key_w / 2, 0, 0]) cube([arm_key_w, arm_hole_d / 2 + arm_key_l, lid_t + 2]);
    }
    // GO button hole, lid thinned from below around it
    translate([go_x, go_y, -1]) cylinder(d = go_hole_d, h = lid_t + 2);
    if (go_panel_t < lid_t)
        translate([go_x, go_y, -1]) cylinder(d = go_relief_d, h = 1 + lid_t - go_panel_t);
    // LED
    translate([led_x, led_y, -1]) cylinder(d = led_hole_d, h = lid_t + 2);
    translate([led_x, led_y, lid_t - 0.6]) cylinder(d1 = led_hole_d, d2 = led_hole_d + 1.2, h = 0.61);
    // top labels
    translate([0, 0, lid_t - label_depth]) linear_extrude(label_depth + 1) {
        translate([arm_x, arm_y - arm_body_d / 2 - 10]) label("ARM", 9);
        translate([go_x, go_y - go_relief_d / 2 - 7]) label("GO", 11);
        translate([led_x, led_y - 9]) label("STATUS", 5);
    }
}

module lid_in_place() {
    difference() {
        union() { lid_plate(); lid_underside(); }
        lid_cuts();
    }
}

module lid_print() {
    // flipped so the top face is on the bed
    translate([0, case_d, lid_t]) rotate([180, 0, 0]) lid_in_place();
}

// ---------------------------------------------------------------------
//  Fit-test coupons
// ---------------------------------------------------------------------
module fit_panel() {
    // the three control holes in a strip of lid, printed top-face down like the lid
    w = 120; d = 56;
    translate([0, d, lid_t]) rotate([180, 0, 0])
    difference() {
        translate([0, 0, 0]) rrect(w, d, lid_t, 4);
        translate([20, d / 2, -1]) {
            cylinder(d = arm_hole_d, h = lid_t + 2);
            if (arm_key_w > 0)
                translate([-arm_key_w / 2, 0, 0]) cube([arm_key_w, arm_hole_d / 2 + arm_key_l, lid_t + 2]);
        }
        translate([70, d / 2, -1]) cylinder(d = go_hole_d, h = lid_t + 2);
        if (go_panel_t < lid_t)
            translate([70, d / 2, -1]) cylinder(d = min(go_relief_d, d - 4), h = 1 + lid_t - go_panel_t);
        translate([108, d / 2, -1]) cylinder(d = led_hole_d, h = lid_t + 2);
        translate([0, 0, lid_t - label_depth]) linear_extrude(1)
            translate([20, 6]) label("ARM", 5);
    }
}

module fit_cradle() {
    // corner of the real base around the board: floor, cradle and back wall with the USB-C cut-out
    intersection() {
        base();
        translate([board_x - 9, pcb_y0 - 6, 0]) cube([pcb_w + 18, case_d - (pcb_y0 - 6) + 1, usb_z + usb_recess_h / 2 + 4]);
    }
}

// ---------------------------------------------------------------------
//  Dummy parts for the assembly preview / clearance checks
// ---------------------------------------------------------------------
module dummy_board() {
    translate([board_x, pcb_y0, pcb_z0]) cube([pcb_w, pcb_l, pcb_t]);
    translate([usb_x - usb_w / 2, pcb_y1 - 7.3 + 0.6, pcb_ztop]) cube([usb_w, 7.3, usb_h]);
}
module dummy_under_lid() {
    z = base_h;  // lid underside in the assembly
    translate([arm_x, arm_y, z - arm_body_depth]) cylinder(d = arm_body_d, h = arm_body_depth);
    translate([go_x, go_y, z - go_body_depth]) cylinder(d = go_body_d, h = go_body_depth);
}
module dummy_jacks() {
    translate([jack_x, case_d - wall - 14, jack_z]) rotate([-90, 0, 0]) cylinder(d = 8, h = 14);
    translate([sma_x, case_d - wall - 10, sma_z]) rotate([-90, 0, 0]) cylinder(d = 8, h = 10);
}

// ---------------------------------------------------------------------
if (part == "base")            base();
else if (part == "lid")        lid_print();
else if (part == "fit_panel")  fit_panel();
else if (part == "fit_cradle") fit_cradle();
else if (part == "lid_in_place") lid_in_place();
else if (part == "dummies")    { dummy_board(); dummy_under_lid(); dummy_jacks(); }
else if (part == "check")      // should render EMPTY: control bodies vs everything in the base
    intersection() {
        union() { dummy_under_lid(); }
        union() { base(); dummy_board(); dummy_jacks(); }
    }
else if (part == "check_board")  // should render EMPTY: board + USB socket vs the base
    intersection() { dummy_board(); base(); }
else if (part == "check_lid")    // should render EMPTY: lid underside vs base, assembled
    intersection() { translate([0, 0, base_h]) lid_in_place(); base(); }
else {  // assembly preview, lid lifted
    color("#2b3a55") base();
    color("#c9a227") dummy_board();
    color("#888") { dummy_under_lid(); dummy_jacks(); }
    color("#3d4f70", 0.9) translate([0, 0, base_h + 40]) lid_in_place();
}
