// Constants //

WallThickness = 1.2;
WallHeightFromPCBSurface = 2;

// AKA, the PCB size minus mounting tabs
DisplayModuleLength      = 38.0;
DisplayModuleWidth       = 32.0;

DisplayModuleThickness   = 1.2;
DisplayScreenBezel       = 1.5;
DisplayBezelThickness    = 0.8;
  
PCBTabExtention_bottom   = 3;
PCBTabExtention_top      = 4;
PCBbottomtabWidth        = 8.5;
PCBbottomtabOffsetA      = 7;
PCBtopTabWidth           = 4;
PCBTabWidthTolerance     = 0.1;
PCBTabLengthTolerance    = 0.1;
PCBFitToleranceLength    = 0.2;
PCBFitToleranceWidth     = 0.12;
PCBThickness             = 1.7; // include tolerance
PCBComponentClearance    = 2.8;

ButtonHoleSize           = 2.8;
ButtonHightFromPCB       = 2.0;
ButtonAOffsetFromPCB     = 9.4; // lengthwise position offset from bottom PCB edge
ButtonBOffsetFromPCB     = 31.4; // lengthwise position offset from bottom PCB edge

RibbonClearanceStart     = 16 - 7; // ribbon cable start edge from sensor side PCB edge
RibbonClearanceEnd       = 16 + 9; // ribbon cable end edge

ThreadInsertHoleDiameter = 3.1;
TopSideThreadHoleDepth  = 9;
ThreadHoleMinHeight     = 6;

USBPortOffsetFromPCB    = 24.7; // lengthwise position offset from bottom PCB edge
USBPortHeight           = 3.4;
USBPortLength           = 9.4;
USBPortRounding         = 1;
USBPortHeightFromPCB    = 1.4; // center distance from PCB surface

BatteryClearanceFromPCB = 7.8;
BatteryLength           = 36; // include tolerance

SensorCornerClearanceWidthwise = 5.5; // includes internal carve-out wall thickness
SensorWallThickness     = 1;
sensorHoleW             = 4.5; 
sensorHoleH             = 2.8; 

boardRetentionTolerance     = 0; // positive for shorter posts, negative for longer posts
retentionPostLengwiseShrink = 0.5; // how much shorter the posts are than the tab width
//retentionPostEdgeTolerance = 0.2; // gap size between post edge and case edge

ScrewHoleDiamter        = 2.2;
screwHolInsetInnterClearance = 3.6; 
screwHoleInsetOuterClearance = 4.0; 

ShellFitTolerance       = 0.1; // tolerance for material going past shell inner edge when clamping fron and back together

///////////////

ep = 0.01;

dispSize = [DisplayModuleLength, DisplayModuleWidth];
disph = DisplayModuleThickness;
bezel = DisplayScreenBezel;
bezelh = DisplayBezelThickness;
screenSize = dispSize[1] - bezel * 2;

tabwa = PCBTabExtention_bottom - PCBTabLengthTolerance;
tabwb = PCBTabExtention_top - PCBTabLengthTolerance;
pcbTol = [PCBFitToleranceLength, PCBFitToleranceWidth];
pcbSize = dispSize + [tabwa + tabwb, 0] + pcbTol * 2;
pcbh = PCBThickness;

pcbSurfaceH = bezelh + disph + pcbh;

wall = WallThickness;

frontPlateTotalH = pcbSurfaceH + WallHeightFromPCBSurface;