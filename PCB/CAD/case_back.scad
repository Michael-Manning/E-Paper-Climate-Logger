include <BOSL2/std.scad>
include <constants.scad>

$fn = 64;

module logo(){
    cylinder(d=7, h=1.2 + ep, center=true);
    translate([7, 0, 0])
    cube([14, 3, 1.2 + ep], center=true);
    translate([14, 0, 0])
    cylinder(d=3, h=1.2 + ep, center=true);

}

module caseBack(){

retentionDip = frontPlateTotalH - pcbSurfaceH - boardRetentionTolerance;

// dependent on case_front.scad sync
bottom_screwSurfaceEdgeA = PCBbottomtabOffsetA - PCBTabWidthTolerance + pcbTol[1] + PCBbottomtabWidth + PCBTabWidthTolerance * 2 + 0.5;
bottom_screwSurfaceEdgeB = PCBbottomtabOffsetA + PCBbottomtabWidth * 2 - PCBTabWidthTolerance + pcbTol[1] - 0.5;
top_screwSurfaceEdgeA = PCBtopTabWidth + PCBTabWidthTolerance + pcbTol[1];
top_screwSurfaceEdgeB = RibbonClearanceStart + pcbTol[1];

bottomScrewPos = [ThreadInsertHoleDiameter / 2 - 0.5, bottom_screwSurfaceEdgeA + (bottom_screwSurfaceEdgeB - bottom_screwSurfaceEdgeA) / 2];
topScrewPos = [tabwb - ThreadInsertHoleDiameter / 2, top_screwSurfaceEdgeA + (top_screwSurfaceEdgeB - top_screwSurfaceEdgeA) / 2];

postWidthwiseTabTolerance = 1.0;

bigCircle = 7;

 // not: battery clearance is  used wrong. 

difference(){

    union(){

        // shell
        difference() {

            union(){
                translate([-wall, -wall, 0])
                cube([
                    pcbSize[0] + wall * 2,  
                    pcbSize[1] + wall * 2, 
                    pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + wall]);
                translate([-wall + screwHoleInsetOuterClearance, -wall + SensorCornerClearanceWidthwise, 0])
                cube([
                    BatteryLength + wall * 2 - screwHoleInsetOuterClearance,  
                    pcbSize[1] + wall * 2 - SensorCornerClearanceWidthwise, 
                    BatteryClearanceFromPCB + wall]);

            }

            difference(){

                union(){
                    translate([0, 0, -ep])
                    cube([
                        pcbSize[0] - tabwb,  
                        pcbSize[1], 
                        pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + ep]);

                    translate([screwHoleInsetOuterClearance, SensorCornerClearanceWidthwise, -ep])
                    cube([
                        BatteryLength - screwHoleInsetOuterClearance,  
                        pcbSize[1] - SensorCornerClearanceWidthwise, 
                        BatteryClearanceFromPCB + ep]);



                    // USB C hole
                    translate([
                        USBPortOffsetFromPCB + pcbTol[0],
                        pcbSize[1] + wall / 2,
                        pcbSurfaceH + USBPortHeightFromPCB - frontPlateTotalH
                    ])
                    cuboid([USBPortLength, wall * 4, USBPortHeight], rounding=USBPortRounding);
                }
            }

        }



        // bottom sensor side retention
        translate([0, SensorCornerClearanceWidthwise, pcbSurfaceH - frontPlateTotalH + boardRetentionTolerance])
        cube([
            tabwa - retentionPostLengwiseShrink, 
            bottom_screwSurfaceEdgeA - SensorCornerClearanceWidthwise - postWidthwiseTabTolerance, 
            pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + retentionDip - boardRetentionTolerance
            ]);

        // bottom other side retention
        translate([0, pcbSize[1] - PCBbottomtabWidth + postWidthwiseTabTolerance, pcbSurfaceH - frontPlateTotalH + boardRetentionTolerance])
        cube([
            tabwa - retentionPostLengwiseShrink, 
            PCBbottomtabWidth - postWidthwiseTabTolerance, 
            pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + retentionDip - boardRetentionTolerance
            ]);

        // // screw guide
        // screwGuideSurfaceMateGap = 0.2;
        // translate([0, bottom_screwSurfaceEdgeA - postWidthwiseTabTolerance, (bezelh + ThreadHoleMinHeight) - frontPlateTotalH + screwGuideSurfaceMateGap])
        // cube([
        //     screwHoleInsetOuterClearance, 
        //     bottom_screwSurfaceEdgeB - bottom_screwSurfaceEdgeA + postWidthwiseTabTolerance * 2, //(PCBbottomtabWidth * 2 - PCBTabWidthTolerance + pcbTol[1]) + postWidthwiseTabTolerance * 2, 
        //     (pcbSurfaceH + PCBComponentClearance - frontPlateTotalH) - ((bezelh + ThreadHoleMinHeight) - frontPlateTotalH) - screwGuideSurfaceMateGap + wall
        //     ]);

        translate([pcbSize[0] - tabwb, 0, 0]){
            // top sensor side retention
            translate([retentionPostLengwiseShrink, 0, pcbSurfaceH - frontPlateTotalH + boardRetentionTolerance])
            cube([
                tabwb - retentionPostLengwiseShrink,
                PCBtopTabWidth - postWidthwiseTabTolerance,
                pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + retentionDip - boardRetentionTolerance
            ]);

            // top other side retention
            translate([retentionPostLengwiseShrink, RibbonClearanceEnd, pcbSurfaceH - frontPlateTotalH + boardRetentionTolerance])
            cube([
                tabwb - retentionPostLengwiseShrink,
                pcbSize[1] - RibbonClearanceEnd,
                pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + retentionDip - boardRetentionTolerance
            ]);
        }


        difference(){

            translate([ButtonAOffsetFromPCB + pcbTol[0], SensorCornerClearanceWidthwise / 2 - wall /2,  pcbSurfaceH + ButtonHightFromPCB - frontPlateTotalH])
            rotate([90, 0, 0])
            cylinder(d=bigCircle + 2, h=SensorCornerClearanceWidthwise + wall, center=true);


            translate([- wall  - ep, - wall -ep, -20])
            cube([100, 100, 20 + pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + ep]);
        }

                difference(){

            translate([ButtonBOffsetFromPCB + pcbTol[0], SensorCornerClearanceWidthwise / 2 - wall /2,  pcbSurfaceH + ButtonHightFromPCB - frontPlateTotalH])
            rotate([90, 0, 0])
            cylinder(d=bigCircle + 2, h=SensorCornerClearanceWidthwise + wall, center=true);


            translate([- wall  - ep, - wall -ep, -20])
            cube([100, 100, 20 + pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + ep]);
        }


        // sensor carve-out
        lx = 7;
        translate([0, SensorCornerClearanceWidthwise - SensorWallThickness, -retentionDip])
        cube([lx, SensorWallThickness, pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + retentionDip]);
        translate([lx - SensorWallThickness, 0, -retentionDip])
        cube([SensorWallThickness, SensorCornerClearanceWidthwise, pcbSurfaceH + PCBComponentClearance - frontPlateTotalH + retentionDip]);
        // fill in the gap above sensor
        translate([0, 0, 0])
        cube([lx - SensorWallThickness, SensorCornerClearanceWidthwise - SensorWallThickness , pcbSurfaceH + PCBComponentClearance - frontPlateTotalH]);

    }
    lx = 7; // sync


    // bottom screw guide
    screwGuideSurfaceMateGap = 0.2;
    translate([0, bottom_screwSurfaceEdgeA - postWidthwiseTabTolerance, 0])
    cube([
        screwHoleInsetOuterClearance + ep, 
        bottom_screwSurfaceEdgeB - bottom_screwSurfaceEdgeA + postWidthwiseTabTolerance * 2, //(PCBbottomtabWidth * 2 - PCBTabWidthTolerance + pcbTol[1]) + postWidthwiseTabTolerance * 2, 
        (bezelh + ThreadHoleMinHeight) - frontPlateTotalH + screwGuideSurfaceMateGap
    ]);
    hardScrewOffset = 0.5;
    translate([
        ThreadInsertHoleDiameter / 2 - hardScrewOffset,
        bottom_screwSurfaceEdgeA + (bottom_screwSurfaceEdgeB - bottom_screwSurfaceEdgeA) / 2,
        0,
    ])
    cylinder(d=ThreadInsertHoleDiameter, h=( (bezelh + ThreadHoleMinHeight) - frontPlateTotalH + screwGuideSurfaceMateGap) * 2, center=true);


    // top screw guide
    epicTolerance = 0.3;
    translate([
        -ep + pcbSize[0] - tabwb,
        top_screwSurfaceEdgeA + 0.1 - epicTolerance, 
        -ep
        ])
    cube([
        tabwb +ep + epicTolerance,
        top_screwSurfaceEdgeB - top_screwSurfaceEdgeA - 0.1 + epicTolerance * 2,
        (bezelh + ThreadHoleMinHeight) - frontPlateTotalH + screwGuideSurfaceMateGap + ep
    ]);

    

    // button holes
    translate([ButtonAOffsetFromPCB + pcbTol[0], -wall / 2,  pcbSurfaceH + ButtonHightFromPCB - frontPlateTotalH])
    rotate([90, 0, 0])
    cylinder(d=ButtonHoleSize, h=wall + 20+ ep, center=true);

    translate([ButtonBOffsetFromPCB + pcbTol[0], -wall / 2,  pcbSurfaceH + ButtonHightFromPCB - frontPlateTotalH])
    rotate([90, 0, 0])
    cylinder(d=ButtonHoleSize, h=wall + 20+ ep, center=true);


    difference(){
        translate([ButtonAOffsetFromPCB + pcbTol[0], 10,  pcbSurfaceH + ButtonHightFromPCB - frontPlateTotalH])
        rotate([90, 0, 0])
        cylinder(d=bigCircle, h=20, center=true);

        translate([0, 0, -20])
        cube([100, 100, 20 + pcbSurfaceH + PCBComponentClearance - frontPlateTotalH - ep]);

        translate([0, 0, -20])
        cube([lx, 100, 100]);
    }
    difference(){
        translate([ButtonBOffsetFromPCB + pcbTol[0], 10,  pcbSurfaceH + ButtonHightFromPCB - frontPlateTotalH])
        rotate([90, 0, 0])
        cylinder(d=bigCircle, h=20, center=true);

        translate([0, 0, -20])
        cube([100, 100, 20 + pcbSurfaceH + PCBComponentClearance - frontPlateTotalH - ep]);

    }



    // sensor hole



    difference(){

        translate([-wall - ep, 0, -ep])
        cube([wall + lx + ep - SensorWallThickness, sensorHoleW, sensorHoleH - (frontPlateTotalH - pcbSurfaceH) + ep]);

        translate([-wall - ep, sensorHoleW / 2, -5])
        cube([10, 0.8, 20], center = true);

    }
    difference(){

        translate([0, -wall - ep, -ep])
        cube([6, wall + ep *2, sensorHoleH - (frontPlateTotalH - pcbSurfaceH) + ep]);

        translate([1.8, -wall - ep, -5])
        cube([0.8, 10, 20], center = true);

        translate([4.2, -wall - ep, -5])
        cube([0.8, 10, 20], center = true);

    }



    // bottom screw guide hole
    translate([bottomScrewPos[0], bottomScrewPos[1], 0])
    cylinder(d=ScrewHoleDiamter, h=100, center=true);

    // top screw guide hole
    translate([topScrewPos[0] + pcbSize[0] - tabwb, topScrewPos[1], 0])
    cylinder(d=ScrewHoleDiamter, h=100, center=true);

    // shell fit tolerance
    difference(){
        translate([-3, -3, -10])
        cube([pcbSize[0] + 3 *2, pcbSize[1] + 3 * 2, 10 ]);
        
        translate([ShellFitTolerance, ShellFitTolerance, -10])
        cube([pcbSize[0] - ShellFitTolerance * 2,  pcbSize[1] - ShellFitTolerance * 2, 11]);
        
    }


    translate([ 14, SensorCornerClearanceWidthwise + (pcbSize[1] - SensorCornerClearanceWidthwise) / 2, BatteryClearanceFromPCB + wall / 2])// SensorCornerClearanceWidthwise + pcbSize[], 0, 0]);
    logo();
}

} // module


color("blue"){
    translate([ 14, SensorCornerClearanceWidthwise + (pcbSize[1] - SensorCornerClearanceWidthwise) / 2, BatteryClearanceFromPCB + wall / 2])// SensorCornerClearanceWidthwise + pcbSize[], 0, 0]);
    logo();
}