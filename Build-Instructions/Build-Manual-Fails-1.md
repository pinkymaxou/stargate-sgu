# Failed prototypes

Many attempt were made before making a functional version. There is a small list of failed prototypes in case you are curious.

## Slip ring attempt

The first attempt used an home made slip ring with 3 tracks, one for GROUND, one for POSITIVE, one for SIGNAL.
It used cooper tape for conductive track, the middle track is also used to guide bearings.
It didn't work because it created too much friction, I tried make my own brushes by cutting graphite but it was a fail. Also conductive glue is surprizingly hard to find or ridiculously expensive so my attempt to make my own conductive glue as also a failure.

![](./Assets/failed-proto-slipring1.png)

![](./Assets/failed-proto-slipring2.png)

## Powered by induction

We successfully lighted up a LED for a few minute then it stopped and never worked again. Anyway it was doomed to fail, we probably just created an illegal emitter by accident in the end.

![](./Assets/failed-proto-induction-1.jpg)

## Friction wheel

Because I couldn't resolve myself to deface the back of the gate by putting a visible gear there I tried using different friction material with a wheel.

Using TPU:

![](./Assets/failed-proto-wheel-1.png)

Using sanding paper:

![](./Assets/failed-proto-wheel-2.png)

## With bearing outside the ring

This one is an epic fail, it could barely old itself and was ridiculously shacky.

![](./Assets/failed-proto-bearingout-1.jpg)

## Use a DC motor

The motor had backlash and not enough mechanical stiffness. Also the encoder didn't have required resolution.
Stepper with 1/16 steps does a better job.

![](./Assets/failed-dc-motor-1.jpg)

## Plunger inside the gate

I wasn't willing to have visible power contact on the backside so I wanted to be able to power recharge using an hidden plunger that connect to the electrical contact hidden inside the ring. 

In the end I betrayed my conviction after it proved too difficult.

![](./Assets/fail-arm-plunger.png)

## Node-MCU

I used a Node-MCU from Aliexpress but it was prone to brown-out. It maybe related to 5V regulator issues.
Still it got replaced by a Lolin32.

## Servo-Motor

I used cheap servo motor for the mecanism but it burned out after a few use. It got replaced by another cheap of the same kind because I didn't have anything else.