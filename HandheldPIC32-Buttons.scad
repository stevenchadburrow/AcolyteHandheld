
// typical buttons
translate([-10,0,0])
{
    difference()
    {
        intersection()
        {
            union()
            {
                cylinder(11,3.75,3.75,center=false);
                cylinder(0.75,5,5,center=false);
            }
            sphere(11);
        }
        cube([2.85,2.85,7],center=true);
       
    }
}

// reset and menu buttons
translate([10,0,0])
{
    difference()
    {
        difference()
        {
            union()
            {
                cylinder(9,3.75,3.75,center=false);
                difference()
                {
                    cylinder(0.75,5,5,center=false);
                    translate([3.75,-5,-1])
                    {
                        cube([10, 10, 10], center = false);
                    }
                }
            }
            translate([0,0,20])
            {
                sphere(12);
            }
        }
        cube([2.85,2.85,7],center=true);
       
    }
}

// no shoulder buttons yet!
