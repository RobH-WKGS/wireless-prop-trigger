// =====================================================================
//  Wireless Prop Trigger — RECEIVER CASE  (sloped desk console)
// ---------------------------------------------------------------------
//  Two-part console: a base tray (electronics) and a control lid
//  (ARM flight switch, GO arcade button, status LED). The top slopes
//  up towards the back (top_angle) so the operator sees the controls
//  face-on. 4 x M3 x 10 socket-head cap screws, perpendicular to the
//  top, into M3 heat-set inserts in the base corner posts.
//
//  Labels are separate INLAY bodies so they can print in a second
//  filament (Prusa XL second tool, or MMU). Each inlay exactly fills
//  its recess in the body, in the same coordinates.
//
//  Parts — set `part`, or from the command line:
//    openscad -D 'part="lid"' -o lid.stl receiver_case.scad
//    "base"        base tray, print floor-down            (colour 1)
//    "base_text"   front + back-panel label inlays        (colour 2)
//    "lid"         control lid, flipped TOP-FACE DOWN     (colour 1)
//    "lid_text"    lid label inlays, same orientation     (colour 2)
//    "fit_panel"   coupon of the lid holes (switch/button/LED fit)
//    "fit_cradle"  corner of the base around the board (USB-C + snap fit)
//    "assembly"    preview with dummy parts (not for printing)
//    "check", "check_board", "check_lid"   clearance checks: render EMPTY
//
//  Everything is in mm. Control positions are measured ALONG the sloped
//  top, from its front edge.
//
//  Label font: Lato Black (WKGS brand). If Lato isn't installed OpenSCAD
//  substitutes a default — set label_font to change it.
// =====================================================================

part = "assembly";   // [base, base_text, lid, lid_text, fit_panel, fit_cradle, assembly]

/* [Case] */
case_w      = 180;   // X, left-right as the operator sees it
case_d      = 115;   // Y, front (0) to back, in plan
front_h     = 42;    // rim height at the front
top_angle   = 12;    // slope of the top, degrees (0 = flat)
lid_t       = 4;     // lid plate thickness
wall        = 3;
floor_t     = 3;
corner_r    = 6;     // outer corner radius (plan view)
fit_gap     = 0.3;   // clearance between lid skirt/bosses and base walls

/* [Fasteners: M3 socket head] */
fastener      = "insert"; // [insert, tap]
insert_d      = 4.0;      // hole for M3 heat-set insert (check your insert's spec)
insert_depth  = 6.5;
tap_d         = 2.6;      // hole for screwing M3 directly into plastic
screw_clear_d = 3.4;
head_d        = 6.2;      // counterbore for M3 SHCS head (5.5 mm head)
head_depth    = 3.4;      // heads sit flush
post_size     = 11;       // square corner posts in the base
boss_h        = 4;        // lid boss depth below the plate (meets the post)

/* [Controls — positions along the sloped top] */
arm_x          = 45;
arm_y          = 48;
arm_hole_d     = 12.2;   // typical missile switch: 12 mm bushing
arm_key_w      = 0;      // anti-rotation key width (0 = none)
arm_key_l      = 1.5;
arm_body_d     = 24;     // clearance envelope under the lid (preview / checks)
arm_body_depth = 38;

go_x           = 135;
go_y           = 54;
go_hole_d      = 30.2;   // 30 mm arcade button (24 mm: 24.2)
go_panel_t     = 2.5;    // lid thinned to this round the button (snap-in buttons need <= ~3)
go_relief_d    = 46;
go_body_d      = 33;
go_body_depth  = 42;

led_x          = 90;
led_y          = 96;
led_hole_d     = 5.3;    // 5 mm through-hole addressable LED
led_rim_d      = 6.2;
led_tube_od    = 9.5;
led_tube_h     = 9;

/* [Board: Challenger RP2040 SubGHz, Feather format] */
pcb_l        = 50.8;
pcb_w        = 22.86;
pcb_t        = 1.6;
pcb_clear    = 0.25;
board_x      = 20;     // left edge of the board (inside the case)
standoff_h   = 3;
usb_w        = 8.94;
usb_h        = 3.26;
usb_z_above_pcb = 1.63;  // centre of receptacle above PCB top
usb_hole_w   = 9.8;
usb_hole_h   = 4.2;
usb_recess_w = 15;     // outside recess for the cable's overmould
usb_recess_h = 9.5;
usb_wall_t   = 1.2;

/* [Back panel] */
sma_x        = 68;
sma_z        = 36;
sma_hole_d   = 6.6;
sma_flat     = 5.9;    // across-flats for D-shaped bulkheads (= sma_hole_d for round)
jack_x       = 112;
jack_z       = 36;
jack_hole_d  = 6.2;

/* [Inside] */
battery_bay  = true;
bay_x        = 66;
bay_y        = 10;
bay_w        = 50;
bay_d        = 36;
bay_wall_h   = 7;
dac_x        = 96;
dac_y        = 72;
dac_w        = 34;
dac_d        = 26;

/* [Labels] */
label_font   = "Lato:style=Black";
label_depth  = 1.0;    // inlay thickness; 1 mm hides a dark body under light text
base_labels  = true;   // front title + back-panel labels on the base
title        = "WIRELESS PROP TRIGGER";

$fn = 64;
eps = 0.01;
BIG = 1000;

// ---------------------------------------------------------------------
//  Derived
// ---------------------------------------------------------------------
ta       = top_angle;
back_h   = front_h + case_d * tan(ta);
lid_len  = case_d / cos(ta);              // length of the top along the slope
H        = back_h + 30;                   // anything taller than the case
pcb_y1   = case_d - wall - 0.2;
pcb_y0   = pcb_y1 - pcb_l;
pcb_z0   = floor_t + standoff_h;
pcb_ztop = pcb_z0 + pcb_t;
usb_x    = board_x + pcb_w / 2;
usb_z    = pcb_ztop + usb_z_above_pcb;
inner_r  = max(corner_r - wall, 0.5);
post_c   = wall + post_size / 2;
post_pos = [[post_c, post_c], [case_w - post_c, post_c],
            [post_c, case_d - post_c], [case_w - post_c, case_d - post_c]];
post_drop = boss_h + 0.2;                 // post tops sit this far below the rim plane

// ---------------------------------------------------------------------
//  Frames: "lid frame" has z = 0 on the rim plane, z = lid_t on the top
//  face, y measured along the slope from the front edge.
// ---------------------------------------------------------------------
module on_lid() { translate([0, 0, front_h]) rotate([ta, 0, 0]) children(); }
module from_lid() { rotate([-ta, 0, 0]) translate([0, 0, -front_h]) children(); }
function lid_y(world_y) = world_y / cos(ta);     // world plan y -> along-slope y

// slab between two planes parallel to the rim plane (lid-frame z values)
module slab(z0, z1) { on_lid() translate([-BIG / 2, -BIG / 2, z0]) cube([BIG, BIG, z1 - z0]); }

// ---------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------
module rrect(w, d, h, r) {
    r2 = max(r, 0.01);
    hull() for (x = [r2, w - r2], y = [r2, d - r2])
        translate([x, y, 0]) cylinder(r = r2, h = h);
}
module rrect_c(w, h, d, r) {   // rounded rect in XZ, centred, extruded along +Y
    rotate([-90, 0, 0]) linear_extrude(d)
        offset(r = r) offset(delta = -r) square([w, h], center = true);
}
module label(txt, size) {
    text(txt, size = size, font = label_font, halign = "center", valign = "center");
}
module footprint(h = H, inset = 0) {
    translate([inset, inset, -1]) rrect(case_w - 2 * inset, case_d - 2 * inset, h + 1, max(corner_r - inset, 0.5));
}

// ---------------------------------------------------------------------
//  BASE
// ---------------------------------------------------------------------
module base_shell() {
    difference() {
        rrect(case_w, case_d, H, corner_r);
        translate([wall, wall, floor_t]) rrect(case_w - 2 * wall, case_d - 2 * wall, H, inner_r);
        slab(0, BIG);                        // cut the top to the slope
    }
}

module corner_posts() {
    intersection() {
        union() for (p = post_pos) {
            cx = p[0] < case_w / 2 ? wall : case_w - wall - post_size;
            cy = p[1] < case_d / 2 ? wall : case_d - wall - post_size;
            translate([cx, cy, 0]) cube([post_size, post_size, H]);
        }
        slab(-BIG, -post_drop);
    }
}

// screw axes are perpendicular to the top
module at_post_axes() {
    for (p = post_pos) on_lid() translate([p[0], lid_y(p[1]), 0]) children();
}

module post_holes() {
    at_post_axes() translate([0, 0, -post_drop]) {
        if (fastener == "insert") {
            translate([0, 0, -insert_depth]) cylinder(d = insert_d, h = insert_depth + 1);
            translate([0, 0, -0.6]) cylinder(d1 = insert_d, d2 = insert_d + 1.2, h = 0.61);
        } else {
            translate([0, 0, -10]) cylinder(d = tap_d, h = 11);
        }
    }
}

module board_cradle() {
    x0 = board_x - pcb_clear;
    x1 = board_x + pcb_w + pcb_clear;
    for (y = [pcb_y0 + 0.5, pcb_y1 - 3.0])
        translate([board_x + 1, y, floor_t - eps]) cube([pcb_w - 2, 2.5, standoff_h + eps]);
    post_t = 1.8; lip = 0.8; lip_h = 1.4;
    for (y = [pcb_y0 + 0.8, pcb_y1 - 4.8], side = [0, 1]) {
        px = side == 0 ? x0 - post_t : x1;
        translate([px, y, floor_t - eps]) {
            cube([post_t, 4, pcb_ztop - floor_t + 0.15 + lip_h]);
            lx = side == 0 ? post_t : -lip;
            translate([lx, 0, pcb_ztop - floor_t + 0.15]) hull() {
                translate([side == 0 ? 0 : lip - eps, 0, 0]) cube([eps, 4, lip_h]);
                cube([lip, 4, 0.4]);
            }
        }
    }
    translate([usb_x - 6, pcb_y0 - pcb_clear - 2.5, floor_t - eps])
        cube([12, 2.5, pcb_ztop - floor_t]);
}

module tie_loop(along_x = false) {
    rotate([0, 0, along_x ? 90 : 0]) difference() {
        translate([-3.5, -2, 0]) cube([7, 4, 4.5]);
        translate([-2.2, -2 - eps, -eps]) cube([4.4, 4 + 2 * eps, 2.4]);
    }
}

module battery_bay_walls() {
    if (battery_bay) translate([bay_x, bay_y, floor_t - eps]) {
        difference() {
            rrect(bay_w + 3.2, bay_d + 3.2, bay_wall_h, 2);
            translate([1.6, 1.6, -1]) rrect(bay_w, bay_d, bay_wall_h + 2, 1);
            translate([4, bay_d, 2]) cube([10, 5, bay_wall_h]);
        }
        for (x = [1.6 + 5, 1.6 + bay_w - 5]) translate([x, 1.6 + bay_d / 2, 0]) tie_loop(along_x = true);
    }
}

module dac_mount() {
    for (x = [dac_x - 4.5, dac_x + dac_w + 4.5])
        translate([x, dac_y + dac_d / 2, floor_t - eps]) tie_loop();
}

module back_panel_holes() {
    translate([usb_x, case_d - wall + usb_wall_t, usb_z]) rrect_c(usb_recess_w, usb_recess_h, wall, 2);
    translate([usb_x, case_d - wall - 1, usb_z]) rrect_c(usb_hole_w, usb_hole_h, wall + 2, 1.2);
    translate([sma_x, case_d - wall - 1, sma_z]) rotate([-90, 0, 0])
        linear_extrude(wall + 2) intersection() { circle(d = sma_hole_d); square([sma_hole_d, sma_flat], center = true); }
    translate([jack_x, case_d - wall - 1, jack_z]) rotate([-90, 0, 0]) cylinder(d = jack_hole_d, h = wall + 2);
}

// base labels; out = how far the shape extends outside the wall (recess: 1, inlay: 0)
module base_label_shapes(out) {
    if (base_labels) {
        // front title, reads from the front
        translate([case_w / 2, label_depth, front_h / 2]) rotate([90, 0, 0])
            translate([0, 0, -out]) linear_extrude(label_depth + out) label(title, 7);
        // back panel, reads from behind
        for (l = [[usb_x, usb_z + usb_recess_h / 2 + 5, "USB-C"],
                  [sma_x, sma_z - 10, "ANT"],
                  [jack_x, jack_z - 10, "LINE OUT"]])
            translate([l[0], case_d - label_depth, l[1]]) rotate([90, 0, 180])
                translate([0, 0, -out]) linear_extrude(label_depth + out) label(l[2], 4.5);
    }
}

module feet_recesses() {
    for (x = [16, case_w - 16], y = [16, case_d - 16])
        translate([x, y, -eps]) cylinder(d = 12.5, h = 1 + eps);
}

module base() {
    difference() {
        union() { base_shell(); corner_posts(); board_cradle(); battery_bay_walls(); dac_mount(); }
        post_holes();
        back_panel_holes();
        base_label_shapes(1);
        feet_recesses();
    }
}

module base_text() { base_label_shapes(0); }

// ---------------------------------------------------------------------
//  LID — built in world coordinates, sitting on the base
// ---------------------------------------------------------------------
module lid_label_shapes(out) {   // lid frame
    translate([0, 0, lid_t - label_depth]) linear_extrude(label_depth + out) {
        translate([arm_x, arm_y - arm_body_d / 2 - 10]) label("ARM", 9);
        translate([go_x, go_y - go_relief_d / 2 - 7]) label("GO", 11);
        translate([led_x, led_y - 9]) label("STATUS", 5);
    }
}

module lid_local_cuts() {   // lid frame
    for (p = post_pos) translate([p[0], lid_y(p[1]), 0]) {
        translate([0, 0, -boss_h - 2]) cylinder(d = screw_clear_d, h = boss_h + lid_t + 4);
        translate([0, 0, lid_t - head_depth]) cylinder(d = head_d, h = head_depth + 1);
    }
    translate([arm_x, arm_y, -1]) {
        cylinder(d = arm_hole_d, h = lid_t + 2);
        if (arm_key_w > 0) translate([-arm_key_w / 2, 0, 0]) cube([arm_key_w, arm_hole_d / 2 + arm_key_l, lid_t + 2]);
    }
    translate([go_x, go_y, -1]) cylinder(d = go_hole_d, h = lid_t + 2);
    if (go_panel_t < lid_t)
        translate([go_x, go_y, -1]) cylinder(d = go_relief_d, h = 1 + lid_t - go_panel_t);
    translate([led_x, led_y, -1]) cylinder(d = led_hole_d, h = lid_t + 2);
    translate([led_x, led_y, lid_t - 0.6]) cylinder(d1 = led_hole_d, d2 = led_hole_d + 1.2, h = 0.61);
    lid_label_shapes(1);
}

module lid_world() {
    iw = case_w - 2 * wall - 2 * fit_gap;
    id = case_d - 2 * wall - 2 * fit_gap;
    ir = max(inner_r - fit_gap, 0.5);
    difference() {
        union() {
            // plate: trimmed to the base footprint so front/back faces are vertical
            intersection() { footprint(); slab(0, lid_t); }
            // locating skirt, vertical walls just inside the base
            intersection() {
                difference() {
                    translate([wall + fit_gap, wall + fit_gap, -1]) rrect(iw, id, H, ir);
                    translate([wall + fit_gap + 1.6, wall + fit_gap + 1.6, -2]) rrect(iw - 3.2, id - 3.2, H + 2, max(ir - 1.6, 0.5));
                }
                slab(-3, eps);
            }
            // corner bosses that land on the posts
            intersection() {
                union() for (p = post_pos) {
                    cx = p[0] < case_w / 2 ? wall + fit_gap : case_w - wall - post_size;
                    cy = p[1] < case_d / 2 ? wall + fit_gap : case_d - wall - post_size;
                    translate([cx, cy, -1]) cube([post_size - fit_gap, post_size - fit_gap, H]);
                }
                translate([wall + fit_gap, wall + fit_gap, -1]) rrect(iw, id, H, ir);
                slab(-boss_h, eps);
            }
            // LED holder tube
            on_lid() translate([led_x, led_y, -led_tube_h]) difference() {
                cylinder(d = led_tube_od, h = led_tube_h + eps);
                translate([0, 0, -1]) cylinder(d = led_rim_d, h = led_tube_h + 2);
            }
        }
        on_lid() lid_local_cuts();
    }
}

module lid_text_world() { intersection() { footprint(); on_lid() lid_label_shapes(0); } }

// print orientation: top face on the bed. The text part uses the same transform.
module to_print() { translate([0, 0, lid_t]) rotate([180, 0, 0]) from_lid() children(); }

// ---------------------------------------------------------------------
//  Fit-test coupons
// ---------------------------------------------------------------------
module fit_panel() {
    w = 120; d = 56;
    translate([0, d, lid_t]) rotate([180, 0, 0]) difference() {
        rrect(w, d, lid_t, 4);
        translate([20, d / 2, -1]) {
            cylinder(d = arm_hole_d, h = lid_t + 2);
            if (arm_key_w > 0) translate([-arm_key_w / 2, 0, 0]) cube([arm_key_w, arm_hole_d / 2 + arm_key_l, lid_t + 2]);
        }
        translate([70, d / 2, -1]) cylinder(d = go_hole_d, h = lid_t + 2);
        if (go_panel_t < lid_t)
            translate([70, d / 2, -1]) cylinder(d = min(go_relief_d, d - 4), h = 1 + lid_t - go_panel_t);
        translate([108, d / 2, -1]) cylinder(d = led_hole_d, h = lid_t + 2);
    }
}

module fit_cradle() {
    intersection() {
        base();
        translate([board_x - 9, pcb_y0 - 6, 0]) cube([pcb_w + 18, case_d - (pcb_y0 - 6) + 1, usb_z + usb_recess_h / 2 + 4]);
    }
}

// ---------------------------------------------------------------------
//  Dummies for preview / checks
// ---------------------------------------------------------------------
module dummy_board() {
    translate([board_x, pcb_y0, pcb_z0]) cube([pcb_w, pcb_l, pcb_t]);
    translate([usb_x - usb_w / 2, pcb_y1 - 7.3 + 0.6, pcb_ztop]) cube([usb_w, 7.3, usb_h]);
}
module dummy_under_lid() {   // bodies hang perpendicular to the top
    on_lid() {
        translate([arm_x, arm_y, -arm_body_depth]) cylinder(d = arm_body_d, h = arm_body_depth);
        translate([go_x, go_y, -go_body_depth]) cylinder(d = go_body_d, h = go_body_depth);
    }
}
module dummy_jacks() {
    translate([jack_x, case_d - wall - 14, jack_z]) rotate([-90, 0, 0]) cylinder(d = 8, h = 14);
    translate([sma_x, case_d - wall - 10, sma_z]) rotate([-90, 0, 0]) cylinder(d = 8, h = 10);
}

// ---------------------------------------------------------------------
if      (part == "base")         base();
else if (part == "base_text")    base_text();
else if (part == "lid")          to_print() lid_world();
else if (part == "lid_text")     to_print() lid_text_world();
else if (part == "lid_world")    lid_world();
else if (part == "lid_text_world") lid_text_world();
else if (part == "fit_panel")    fit_panel();
else if (part == "fit_cradle")   fit_cradle();
else if (part == "dummies")      { dummy_board(); dummy_under_lid(); dummy_jacks(); }
else if (part == "check")        intersection() { dummy_under_lid(); union() { base(); dummy_board(); dummy_jacks(); } }
else if (part == "check_board")  intersection() { dummy_board(); base(); }
else if (part == "check_lid")    intersection() { translate([0, 0, 0.01]) lid_world(); base(); }  // 0.01: lid rests on the rim
else if (part == "check_text")   union() {  // inlays vs bodies: should render EMPTY
    intersection() { base(); base_text(); }
    intersection() { lid_world(); lid_text_world(); }
}
else {
    color("#283341") base();
    color("#c09e50") base_text();
    color("#9a9a9a") { dummy_board(); dummy_under_lid(); dummy_jacks(); }
    translate([0, 0, 40]) { color("#34435a") lid_world(); color("#c09e50") lid_text_world(); }
}
