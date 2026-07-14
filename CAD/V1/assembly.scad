include <BOSL2/std.scad>
include <constants.scad>
use <case_front.scad>
use <case_back.scad>

$fn = 40;


color("grey"){
    caseFront();

    translate([0, 0, frontPlateTotalH + 0])
    caseBack();
}


translate([170.6, -39.5, 2])
rotate([0, 0, 180])
import("templog_mainboard.stl");


