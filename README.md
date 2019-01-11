## What
Baby AUV is a low cost, open source micro AUV (autonomous
underwater vehicle) intended for environmental monitoring
applications.  Currently it has completed basic manoeuvring and
navigation tasks, and the payload sensors (CTD) is functioning.

To move forward, the project needs a real-world problem and a
domain expert to guide further development and to take the AUV
operational.  Contact me htarold@gmail.com if you think Baby AUV
can be useful for you, or if you are interested in working on
the project.  Ideally a community can be grown to handle the
development and operational issues.

At least initially, Baby AUV should be deployed in inland waters
free of current.  Ultimately, Baby AUV will be useable in
coastal waters.

You can see a clip of Baby AUV [here](https://www.youtube.com/watch?v=7lgA8mxeRek).

## Vital statistics
Length: 0.6m (1m including antenna)
Displacement: 3.5kg
Cost: BOM cost can probably be held down to USD500.
Depth: the thruster gland has been tested to 40m; the EC probe
is rated to 50m; the hull is probably good to a few hundred
metres.
Endurance: 48 hours
Speed: 2kt (this implies range is about 100 miles)
Vehicle sensors: GPS, compass, accelerometer, depth, odometer
(via propeller)
Payload sensors: electrical conductivity, temperature.
Other payloads can be added.
Communications: RF modem (434MHz).

## How Baby AUV is different
Unlike most AUVs, Baby AUV has no fins, and has only 1 thruster,
and a moving mass actuator (2 degrees of freedom).  But by keeping
it mechanically simple and therefore cheap, it has to employ
novel methods to turn.  Gross changes of direction are achieved by
stopping, pitching nose up (by moving its internals to the rear),
then slowly reversing the thruster.  While the vehicle slowly
descends, it also rolls about a vertical axis as it reacts to the
thruster torque. At the appropriate point, the thruster is stopped
and the AUV is pitched horizontal, pointing in a new direction.

While under way, small changes in heading are achieved by modulating
the torque to the propeller which has only one blade.  This tends to
walk the rear of the AUV slowly one way or the other.  Another way
to think about this is the asymmetry of the single bladed prop
causes the AUV to wobble slightly in normal use.  By appropriately
modulating it, the wobble can be biased more one way than the other.

It should also be clear that Baby AUV is capable of hovering.

Baby AUV depends on GPS for localisation.  This means that the
AUV must surface frequently in order to get a fix.
Conveniently, for most missions the surface is very close by.

Most AUV projects are first and foremost robotics projects.
By keeping *this* AUV simple, I hope to de-emphasise the robotics
aspect in order to let the data sampling take centre stage.  In a
sense, Baby AUV can be regarded as a self-propelled multiparameter
probe, where one of the sensors s replaced by a thruster.

## Missions

The relatively high endurance and long range makes it suitable
for prolonged monitoring missions.  The AUV visits the waypoints
indicated in a mission script, collecting data along the way.
There data are stored on the SD card, and are also opportunistically
broadcast when surfaced.  If out of radio range of "home base", any
other AUV or aerial drone in the area can copy this data to return
to the user.  (A [fountain code](https://en.wikipedia.org/wiki/Fountain_code) will be used to reduce the amount
of redundant data transmitted.)  Use of such data mules make data
available sooner, as well as extending the AUV's sampling domain.

Apart from using the CTD or other payload sensor, one novel use is
to employ the AUV itself as a hydrometric pendulum.  To do this, the
AUV is trimmed slightly negative (tending to sink instead of float),
and is parked on the bottom balancing on its nose.  Any current near
the bottom (not powerful enough to dislodge the AUV) will cause
the AUV to change pitch and heading like an upside down windsock;
these are picked up by the vehicle's compass and accelerometer.
This can provide a simple way to measure the bottom shear (e.g. for
sediment transport studies), which is otherwise expensive to measure.

Baby AUV's hovering capability can be used to turn it into a
self-deploying and self-recovering fixed depth Lagrangean drifter.

Baby AUV can be used in a high surface traffic area.  The small size,
low speed, and low cost means that the AUV will sacrificially lose
in any collision with another vessel.  Conveniently, when surfaced
the AUV will be vertical, which pose softens impacts.
