// RPG Unify Box
// Jack Shaver
// 3-15-2025

module pcbKeepout(){
    difference(){
        translate([0,0,0])cube([100,100,1.6]); // pcb
        
        translate([4,23,-6])cylinder(d=2.5, h=10, $fn=16); // holes
        translate([96,23,-6])cylinder(d=2.5, h=10, $fn=16);
        translate([4,96,-6])cylinder(d=2.5, h=10, $fn=16);
        translate([96,96,-6])cylinder(d=2.5, h=10, $fn=16);
    }
    
    //translate([7,-3,1.6])cube([86,21,29]); // rear connectors
    
    
    //translate([17.9,93,1.3])cube([9.6,10,10.2]); // leds
    //translate([13.2,93,6.5])rotate([-90,0,0])cylinder(d=8,h=10,$fn=16); // power button
    //translate([32,93,6.5])rotate([-90,0,0])cylinder(d=8,h=10,$fn=16); // fire button
    
    translate([8,93,1.3])cube([29,10,10.2]);
    
    
    translate([29.4,93,1.6])cube([5.2,10,5]);// fire button
    translate([10.6,93,1.6])cube([5.2,10,5]);// fire button
    
    //translate([65.25,93,1.3])cube([9.5,10,3.8]); // type c
    //translate([78,93,1.6])cube([12,10,2.5]); // micro sd
    translate([65,93,1.3])cube([25,10,4]); // type c and micro sd

}

module lidMount(height, holeDiameter){
    difference(){
        cube([8,8,height]);
        translate([4,4,0])cylinder(d=holeDiameter,h=10,$fn=16);
    }
}

module keyHole(){
    difference(){
        rotate([-90,0,0])cylinder(d=12,h=6,$fn=16);
        translate([-15,0,-6])cube([30,10,1]);
        translate([-15,0,5])cube([30,10,1]);
    }
}   

module lid(){
    difference(){
        translate([-2,-2,0])cube([104,104,24]);
        translate([-0.5,-0.5,0])cube([101,101,22]);
        translate([0,0,-3])pcbKeepout();
        translate([50,98,10])keyHole();
        translate([90,90,0])cylinder(d=6.5,h=50,$fn=16);
    }
    
    translate([-10,-2,0])lidMount(2,3);
    translate([102,-2,0])lidMount(2,3);
    translate([-10,94,0])lidMount(2,3);
    translate([102,94,0])lidMount(2,3);
}

module enclosure(){
    difference(){
        translate([-2,-2,0])cube([104,104,9]);
        translate([0,0,6])pcbKeepout();
        translate([-0.5,-0.5,1])cube([101,101,10]);
        translate([4,23,1])cylinder(d=2, h=10, $fn=16); // holes
        translate([96,23,1])cylinder(d=2, h=10, $fn=16);
        translate([4,96,1])cylinder(d=2, h=10, $fn=16);
        translate([96,96,1])cylinder(d=2, h=10, $fn=16);
    }
    
    difference(){
       translate([-2,-2,0])cube([104,104,6]);
       translate([8,-2,0])cube([84,120,12]);
       translate([-2,27,0])cube([120,65,12]); 
       translate([-2,-2,0])cube([120,21,12]); 
        
       translate([4,23,1])cylinder(d=2, h=10, $fn=16); // holes
       translate([96,23,1])cylinder(d=2, h=10, $fn=16);
       translate([4,96,1])cylinder(d=2, h=10, $fn=16);
       translate([96,96,1])cylinder(d=2, h=10, $fn=16);
       //translate([0,0,6])pcbKeepout();
    }
    
    translate([-10,-2,0])lidMount(9,2);
    translate([102,-2,0])lidMount(9,2);
    translate([-10,94,0])lidMount(9,2);
    translate([102,94,0])lidMount(9,2);
    
    translate([-10,-2,0])cube([8,8,1]);
    translate([102,-2,0])cube([8,8,1]);
    translate([-10,94,0])cube([8,8,1]);
    translate([102,94,0])cube([8,8,1]);
    
}

module button(){
    difference(){
        cylinder(d=8,h=6,$fn=16);
        translate([-1.5,-1.5,3.5])cube([3,3,3]);
    }
}

module display(){
    //pcbKeepout();
    enclosure();
    //lid();
    //keyHole();
    translate([40,-10,0])button();
    translate([60,-10,0])button();
    translate([0,-20,24])rotate([180,0,0])lid();
}

display();