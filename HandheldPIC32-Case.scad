h = 40; // total height of case
c = 15; // top height of the case

difference()
{
    translate([0, 0, c-h])
    {
        difference()
        {
            union()
            {
                // outside of case, good
                intersection()
                {
                    translate([-85, 45, 0])
                    { 
                        cylinder(h, 12, 12, center=false);
                    }
                    translate([-85, -57, h/2])
                    {
                        rotate([-90, 0, 0])
                        {
                            cylinder(114, h/2, h/2, center=false);
                        }
                    }
                }
                intersection()
                {
                    translate([-85, -45, 0])
                    { 
                        cylinder(h, 12, 12, center=false);
                    }
                    translate([-85, -57, h/2])
                    {
                        rotate([-90, 0, 0])
                        {
                            cylinder(114, h/2, h/2, center=false);
                        }
                    }
                }
                intersection()
                {
                    translate([85, 45, 0])
                    { 
                        cylinder(h, 12, 12, center=false);
                    }
                    translate([85, -57, h/2])
                    {
                        rotate([-90, 0, 0])
                        {
                            cylinder(114, h/2, h/2, center=false);
                        }
                    }
                }
                intersection()
                {
                    translate([85, -45, 0])
                    { 
                        cylinder(h, 12, 12, center=false);
                    }
                    translate([85, -57, h/2])
                    {
                        rotate([-90, 0, 0])
                        {
                            cylinder(114, h/2, h/2, center=false);
                        }
                    }
                }
                translate([-87, -57, 0])
                {
                    cube([174, 114, h], center=false);
                }
                intersection()
                {
                    translate([-97, -47, 0])
                    { 
                        cube([14, 94, h], center=false);
                    }
                    translate([-85, -55, h/2])
                    {
                        rotate([-90, 0, 0])
                        {
                            cylinder(114, h/2, h/2, center=false);
                        }
                    }
                }
                intersection()
                {
                    translate([83, -47, 0])
                    { 
                        cube([14, 94, h], center=false);
                    }
                    translate([85, -55, h/2])
                    {
                        rotate([-90, 0, 0])
                        {
                            cylinder(114, h/2, h/2, center=false);
                        }
                    }
                }
                difference() // programming pin protector, added
                {
                    translate([-80, -55, h-c-8.5])
                    {
                        rotate([45,0,0])
                        {
                            cube([16, 10, 10], center=false);
                        }
                    }
                    translate([-81, -100-40, h-c-3])
                    {
                        cube([18, 100, 3], center=false);
                    }
                }
            }
            union()
            {
                // inside of case, good
                translate([-80, 40, 5])
                { 
                    cylinder(h-10, 12, 12, center=false);
                }
                translate([-80, -40, 5])
                { 
                    cylinder(h-10, 12, 12, center=false);
                }
                translate([80, 40, 5])
                { 
                    cylinder(h-10, 12, 12, center=false);
                }
                translate([80, -40, 5])
                { 
                    cylinder(h-10, 12, 12, center=false);
                }
                translate([-82, -52, 5])
                {
                    cube([164, 104, h-10], center=false);
                }
                translate([-92, -42, 5])
                {
                    cube([14, 84, h-10], center=false);
                }
                translate([78, -42, 5])
                {
                    cube([14, 84, h-10], center=false);
                }
                
                // screw holes, good
                translate([-80, 40, h-5])
                {
                    cylinder(20, 3, 3, center=false);
                }
                translate([-80, -40, h-5])
                {
                    cylinder(20, 3, 3, center=false);
                }
                translate([0, 40, h-5])
                {
                    cylinder(20, 3, 3, center=false);
                }
                translate([0, -40, h-5])
                {
                    cylinder(20, 3, 3, center=false);
                }
                translate([80, 40, h-5])
                {
                    cylinder(20, 3, 3, center=false);
                }
                translate([80, -40, h-5])
                {
                    cylinder(20, 3, 3, center=false);
                }
                
                // button holes, good
                translate([-70, 33, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([-78, 25, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([-70, 17, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([-62, 25, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([70, 33, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([78, 25, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([70, 17, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([62, 25, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([-6, -30, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([6, -30, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([52, -10, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                translate([52, -22, h-c])
                {
                    cylinder(100, 4.25, 4.25, center=false);
                }
                
                // led holes, good
                translate([61.735, -10, h-c])
                {
                    cylinder(100, 3, 3, center=false);
                }
                translate([61.735, -22, h-c])
                {
                    cylinder(100, 3, 3, center=false);
                }
                
                // shoulder button holes, fixed
                translate([-75, 39.25, h-c-9])
                {
                    cube([10, 7.5, 10], center=false);
                }
                translate([-70-10, 39.25, h-c-9])
                {
                    cube([20, 20, c-5], center=false);
                }
                translate([65, 39.25, h-c-9])
                {
                    cube([10, 7.5, 10], center=false);
                }
                translate([70-10, 39.25, h-c-9])
                {
                    cube([20, 20, c-5], center=false);
                }
                
                // screen hole, fixed
                translate([-35, -20, h-c])
                {
                    cube([72, 55, 100], center=false);
                }
                
                // speaker holes, good
                translate([-55, -30, h-c])
                {
                    cylinder(7.5, 15, 15, center=false);
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-60, -30, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-50, -30, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-55, -25, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-55, -35, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-60, -25, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-50, -25, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-60, -35, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-50, -35, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-65, -30, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-45, -30, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-55, -20, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                translate([-55, -40, h-c])
                {
                    cylinder(100, 1.5, 1.5, center=false);
                }
                
                // top row holes, individual
                translate([-56, 40, h-c-6])
                {
                    cube([15, 100, 8], center=false); // fixed
                }
                translate([-31, 40, h-c-9.5])
                {
                    rotate([-90, 0, 0])
                    {
                        cylinder(100, 4, 4, center=false); // fixed
                    }
                }
                translate([-12.5, 40, h-c-3.25])
                {
                    rotate([-90, 0, 0])
                    {
                        cylinder(100, 7, 7, center=false); // fixed
                    }
                }
                translate([5, -40, h-c-14])
                {
                    cube([33, 100, 14], center=false); // good
                }
                translate([43, 40, h-c-8.5])
                {
                    cube([12, 100, 6], center=false); // good
                }
                
                // bottom row holes, individual
                translate([-80, -100-40, h-c-3])
                {
                    cube([16, 100, 3], center=false); // fixed
                }
                translate([-60, -100-40, h-c-3])
                {
                    cube([18, 100, 3], center=false); // fixed
                }
                translate([-38, -100-40, h-c-14])
                {
                    cube([33, 100, 14], center=false); // good
                }
                translate([5, -100-40, h-c-14])
                {
                    cube([33, 100, 14], center=false); // good
                }
                translate([42, -100-40, h-c-8])
                {
                    cube([18, 100, 8], center=false); // fixed
                }
                translate([59, -100-40, h-c-18])
                {
                    cube([20, 100, 26], center=false); // needs work!
                }
            }
        }
        difference()
        {
            union()
            {
                // button containers, fixed
                translate([-70, 33, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([-78, 25, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([-70, 17, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([-62, 25, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([70, 33, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([78, 25, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([70, 17, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([62, 25, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([-6, -30, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([6, -30, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([52, -10, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                translate([52, -22, h-c+7.5])
                {
                    difference()
                    {
                        cylinder(c-7.5, 6.25, 6.25, center=false);
                        cylinder(c-7.5, 4.25, 4.25, center=false);
                    }
                }
                
                // led containers, fixed
                translate([61.735, -10, h-c+3.5])
                {
                    difference()
                    {
                        cylinder(c-3.5, 5, 5, center=false);
                        cylinder(c-3.5, 3, 3, center=false);
                    }
                }
                translate([61.735, -22, h-c+3.5])
                {
                    difference()
                    {
                        cylinder(c-3.5, 5, 5, center=false);
                        cylinder(c-3.5, 3, 3, center=false);
                    }
                }
            
                // screw studs, good
                translate([-80, 40, 5])
                {
                    difference()
                    {
                        cylinder(h-10, 5, 5, center=false);
                        cylinder(h-10, 1.5, 1.5, center=false);
                    }
                    translate([-12, -2.5, 0])
                    {
                        cube([9.5, 5, h-10], center=false);
                    }
                }
                translate([-80, -40, 5])
                {
                    difference()
                    {
                        cylinder(h-10, 5, 5, center=false);
                        cylinder(h-10, 1.5, 1.5, center=false);
                    }
                    translate([-12, -2.5, 0])
                    {
                        cube([9.5, 5, h-10], center=false);
                    }
                }
                translate([0, 40, 5])
                {
                    difference()
                    {
                        cylinder(h-10, 5, 5, center=false);
                        cylinder(h-10, 1.5, 1.5, center=false);
                    }
                    translate([-2.5, 2.5, 0])
                    {
                        cube([5, 9.5, h-10], center=false);
                    }
                }
                translate([0, -40, 5])
                {
                    difference()
                    {
                        cylinder(h-10, 5, 5, center=false);
                        cylinder(h-10, 1.5, 1.5, center=false);
                    }
                    translate([-2.5, -12, 0])
                    {
                        cube([5, 9.5, h-10], center=false);
                    }
                }
                translate([80, 40, 5])
                {
                    difference()
                    {
                        cylinder(h-10, 5, 5, center=false);
                        cylinder(h-10, 1.5, 1.5, center=false);
                    }
                    translate([2.5, -2.5, 0])
                    {
                        cube([9.5, 5, h-10], center=false);
                    }
                }
                translate([80, -40, 5])
                {
                    difference()
                    {
                        cylinder(h-10, 5, 5, center=false);
                        cylinder(h-10, 1.5, 1.5, center=false);
                    }
                    translate([2.5, -2.5, 0])
                    {
                        cube([9.5, 5, h-10], center=false);
                    }
                }
            }
            union()
            {
                // inside of case (pcb height), good
                translate([-80, 40, h-c])
                { 
                    cylinder(1.75, 12, 12, center=false);
                }
                translate([-80, -40, h-c])
                { 
                    cylinder(1.75, 12, 12, center=false);
                }
                translate([80, 40, h-c])
                { 
                    cylinder(1.75, 12, 12, center=false);
                }
                translate([80, -40, h-c])
                { 
                    cylinder(1.75, 12, 12, center=false);
                }
                translate([-82, -52, h-c])
                {
                    cube([164, 104, 1.75], center=false);
                }
                translate([-92, -42, h-c])
                {
                    cube([14, 84, 1.75], center=false);
                }
                translate([78, -42, h-c])
                {
                    cube([14, 84, 1.75], center=false);
                }
            }
        }
    }
    
    /*
    // uncomment to remove bottom
    difference()
    {
        translate([-500, -500, -500])
        {
            cube([1000, 1000, 500], center=false);
        }
    }
    */
    /*
    // uncomment to remove top
    union()
    {
        translate([-500, -500, 0])
        {
            cube([1000, 1000, 500], center=false);
        }
    }
    */
}






