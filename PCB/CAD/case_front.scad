include <BOSL2/std.scad>
include <constants.scad>

$fn = 64;

module caseFront(){
        innd = 5.4;
        thikk = 2.6;
difference(){

    union(){

        // shell
        difference() {

            translate([-wall, -wall, 0])
            cube([
                pcbSize[0] + wall * 2,  
                pcbSize[1] + wall * 2, 
                frontPlateTotalH]);

            translate([0, 0, bezelh])
            cube([
                pcbSize[0], 
                pcbSize[1], 
                frontPlateTotalH]);

            // screen cutout
            translate([
                tabwa + pcbTol[0] +  bezel, 
                pcbTol[1] + bezel, 
                -ep /2])

            cube([
                screenSize, 
                screenSize, 
                bezelh + ep
                ]);

            // button holes
            translate([ButtonAOffsetFromPCB + pcbTol[0], -wall / 2,  pcbSurfaceH + ButtonHightFromPCB])
            rotate([90, 0, 0])
            cylinder(d=ButtonHoleSize, h=wall + 20+ ep, center=true);

            translate([ButtonBOffsetFromPCB + pcbTol[0], -wall / 2,  pcbSurfaceH + ButtonHightFromPCB])
            rotate([90, 0, 0])
            cylinder(d=ButtonHoleSize, h=wall + 20+ ep, center=true);

            // USB C hole
            translate([
                USBPortOffsetFromPCB + pcbTol[0],
                pcbSize[1] + wall / 2,
                pcbSurfaceH + USBPortHeightFromPCB
            ])
            cuboid([USBPortLength, wall * 4, USBPortHeight], rounding=USBPortRounding);

            // sensor hole
            difference(){

                translate([-wall - ep, 0, pcbSurfaceH])
                cube([wall + ep *2, sensorHoleW, sensorHoleH]);

                translate([-wall - ep, sensorHoleW / 2, pcbSurfaceH])
                cube([10, 0.8, 10], center = true);

            }
            difference(){

                translate([0, -wall - ep, pcbSurfaceH])
                cube([6, wall + ep *2, sensorHoleH]);

                translate([1.8, -wall - ep, pcbSurfaceH])
                cube([0.8, 10, 10], center = true);

                translate([4.2, -wall - ep, pcbSurfaceH])
                cube([0.8, 10, 10], center = true);

            }



            screwSurfaceEdgeA = PCBbottomtabOffsetA - PCBTabWidthTolerance + pcbTol[1] + PCBbottomtabWidth + PCBTabWidthTolerance * 2 + 0.5;
            screwSurfaceEdgeB = PCBbottomtabOffsetA + PCBbottomtabWidth * 2 - PCBTabWidthTolerance + pcbTol[1] - 0.5;
            // thread insert hole
            hardScrewOffset = 0.5;
            translate([
                ThreadInsertHoleDiameter / 2 - hardScrewOffset,
                screwSurfaceEdgeA + (screwSurfaceEdgeB - screwSurfaceEdgeA) / 2,
                bezelh + ThreadHoleMinHeight / 2 + ep / 2,
            ])
            cylinder(d=ThreadInsertHoleDiameter, h=ThreadHoleMinHeight + ep, center=true);
        };

        // bottom PCB insert tabs
        translate([0, 0, bezelh])
        difference(){

            screwSurfaceEdgeA = PCBbottomtabOffsetA - PCBTabWidthTolerance + pcbTol[1] + PCBbottomtabWidth + PCBTabWidthTolerance * 2 + 0.5;
            screwSurfaceEdgeB = PCBbottomtabOffsetA + PCBbottomtabWidth * 2 - PCBTabWidthTolerance + pcbTol[1] - 0.5;

            union(){
                translate([0, 0, 0])
                cube([
                    tabwa,
                    PCBbottomtabOffsetA - PCBTabWidthTolerance + pcbTol[1], 
                    disph + pcbh
                ]);
                translate([0, 0, 0])
                cube([
                    tabwa + 0.2,
                    screwSurfaceEdgeA, 
                    disph
                ]);

                translate([0, screwSurfaceEdgeA, 0])
                cube([
                    tabwa,
                    screwSurfaceEdgeB - screwSurfaceEdgeA, 
                    ThreadHoleMinHeight
                ]);
                translate([0, screwSurfaceEdgeB, 0])
                cube([
                    tabwa  + 0.2,
                    pcbSize[1] - screwSurfaceEdgeB, 
                    disph
                ]);
            }

            // thread insert hole
            hardScrewOffset = 0.5;
            translate([
                ThreadInsertHoleDiameter / 2 - hardScrewOffset,
                screwSurfaceEdgeA + (screwSurfaceEdgeB - screwSurfaceEdgeA) / 2,
                bezel + ThreadHoleMinHeight - TopSideThreadHoleDepth / 2 + ep / 2,
            ])
            cylinder(d=ThreadInsertHoleDiameter, h=TopSideThreadHoleDepth + ep, center=true);

            // remove thin edge on hole
            translate([
                ThreadInsertHoleDiameter,
                screwSurfaceEdgeA + (screwSurfaceEdgeB - screwSurfaceEdgeA) / 2,
                bezelh + ThreadHoleMinHeight - TopSideThreadHoleDepth / 2 + ep / 2,
            ])
            cube([ThreadInsertHoleDiameter, ThreadInsertHoleDiameter / 2, 100], center=true);
            translate([
                0,
                screwSurfaceEdgeA + (screwSurfaceEdgeB - screwSurfaceEdgeA) / 2,
                ThreadHoleMinHeight,
            ])
            cube([ThreadInsertHoleDiameter, ThreadInsertHoleDiameter / 2, (ThreadHoleMinHeight - frontPlateTotalH + bezelh) *2   ], center=true);
        }

        // top PCB insert tabs
        translate([pcbSize[0] - tabwb, 0, bezelh])
        difference(){

            screwSurfaceEdgeA = PCBtopTabWidth + PCBTabWidthTolerance + pcbTol[1];
            screwSurfaceEdgeB = RibbonClearanceStart + pcbTol[1];

            union(){
                // sensor side
                translate([0, 0, 0])
                cube([
                    tabwb,
                    screwSurfaceEdgeA + 0.1, 
                    disph
                ]);
                translate([0, screwSurfaceEdgeA + 0.1, 0])
                cube([
                    tabwb,
                    screwSurfaceEdgeB - screwSurfaceEdgeA - 0.1,
                    ThreadHoleMinHeight
                ]);

                // other side
                translate([0, RibbonClearanceEnd, 0])
                cube([
                    tabwb,
                    pcbSize[1] - RibbonClearanceEnd,
                    disph
                ]);
                translate([0, RibbonClearanceEnd, 0])
                cube([
                    tabwb,
                    pcbSize[1] - RibbonClearanceEnd - PCBtopTabWidth - PCBTabWidthTolerance,
                    disph + pcbh
                ]);
            }


            // thread insert hole
            translate([
                tabwb - ThreadInsertHoleDiameter / 2,
                screwSurfaceEdgeA + (screwSurfaceEdgeB - screwSurfaceEdgeA) / 2,
                bezelh + ThreadHoleMinHeight - TopSideThreadHoleDepth / 2 + ep / 2,
            ])
            cylinder(d=ThreadInsertHoleDiameter, h=TopSideThreadHoleDepth + ep, center=true);

            // remove thin edge on hole
            translate([
                tabwb,
                screwSurfaceEdgeA + (screwSurfaceEdgeB - screwSurfaceEdgeA) / 2,
                ThreadHoleMinHeight,
            ])
            cube([ThreadInsertHoleDiameter, ThreadInsertHoleDiameter / 2, (ThreadHoleMinHeight - frontPlateTotalH + bezelh) *2   ], center=true);
            
        }

        difference(){

            translate([
                pcbSize[0] + wall + innd / 2 + thikk,
                (pcbSize[1] + wall * 2) / 2 - wall,
                frontPlateTotalH / 2
            ])
            cylinder(d=innd, h=frontPlateTotalH, center=true);

            translate([
                pcbSize[0] + wall + innd / 2 + thikk,
                0,
                -ep
            ])
            cube([20,20, 20]);
        }

    } // union

    difference(){
        translate([-3, -3, frontPlateTotalH])
        cube([pcbSize[0] + 3 *2, pcbSize[1] + 3 * 2, 10 ]);
        
        translate([ShellFitTolerance, ShellFitTolerance, frontPlateTotalH - ep])
        cube([pcbSize[0] - ShellFitTolerance * 2,  pcbSize[1] - ShellFitTolerance * 2, 11]);
        
    }

} // difference


        cavity = 4.1;
        tabt = (frontPlateTotalH - cavity) / 2;


        difference(){
            union(){

                difference(){

                    translate([
                        pcbSize[0] + wall,
                        (pcbSize[1] + wall * 2) / 2 - wall - (innd ) /2,
                        0
                    ])
                    cube([innd / 2 + thikk , innd, frontPlateTotalH]);

                    // translate([
                    //     pcbSize[0] + wall,
                    //     (pcbSize[1] + wall * 2) / 2 - wall - (innd ) /2 - ep,
                    //     tabt
                    // ])
                    // cube([innd / 2 + thikk , innd + ep * 2, cavity]);
                }


                translate([pcbSize[0] + wall,
                        (pcbSize[1] + wall * 2) / 2 - wall - (innd ) /2,
                        0]){
                    linear_extrude(height = frontPlateTotalH)
                    polygon(points=[[0,0], [0,-5], [(innd / 2 + thikk),0]]);
                }
                translate([pcbSize[0] + wall,
                        (pcbSize[1] + wall * 2) / 2 - wall + (innd ) /2,
                        0]){
                    linear_extrude(height = frontPlateTotalH)
                    polygon(points=[[0,0], [0,5], [(innd / 2 + thikk),0]]);
                }
            }

                        translate([
                pcbSize[0] + wall + innd / 2 + thikk,
                (pcbSize[1] + wall * 2) / 2 - wall,
                frontPlateTotalH / 2
            ])
            cylinder(d=innd + thikk * 2, h=cavity, center=true);

        }

         translate([
            pcbSize[0] + wall + 7,
            (pcbSize[1] + wall * 2) / 2 - wall - (innd ) /2 - 8,
            0
        ])
        cube([innd / 2 + thikk + 7 , innd + 16, tabt]);

} // module

color("grey"){
caseFront();
}