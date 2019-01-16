
Wed 16 Jan 11:31:11 +08 2019

## Mechanical drawings

These 2D drawings were done on librecad.
I still need to properly annotate these drawings.

### `antenna.dxf`
This is for the antenna radome.  The radome contains 2 antennas
proper, for GPS and 433MHz modem.  A CNC
router (or you can use a milling machine) is used to rout out
the pocket and outline out of PVC.  Then a lathe is used to turn the
nipple, the 433MHz section, and to drill the holes for the
433MHz spring and nipple.  You may be able to get by with using
a drill press for this but it's rather dangerous.

The GPS antenna is an off-the-shelf active small ceramic patch
antenna.  The tiny screening
can over the LNA needs to be removed in order to replace the IPX
coax cable with a longer one (the can being replaced after that).
The coax cable _must be threaded_ through the antenna radome before
this is done!  Similarly for the 433MHz coax cable.  Note the
433MHz spring coax cable must be routed _behind_ the GPS antenna,
so that it lies against the screen can of the LNA, and the GPS
patch antenna must be installed facing the pocket opening.
Then the radome caps can be cemented in place with PVC cement.
You can the do the leak test.

### `baby.dxf`
Main drawing for all parts associated with the hull assembly.

The bulkhead sealing deserves a few words.  The two bulkheads take
a Neoprene ring of 5-6mm square cross section, which is compressed
against the hull by the 6 M4 screws.  This makes a water tight seal,
and also firmly attaches the bulkhead to the hull, obviating the
need for screws.  The Neoprene ring is made from a strip of the
appropriate length, by gluing the ends together in the same way
that O-rings can be made from O-ring cord.  My old source for
Neoprene strip has disappeared and I haven't looked for a new
one, but Neoprene from any source can be used.

The bulkheads themselves are turned on a lathe.  Most machining
work is concentrated here.

Small brackets are generally made from aluminium angle stock.

### `tracker.dxf`
Parts for the GPS tracker.
