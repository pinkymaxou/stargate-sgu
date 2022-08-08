# Base electronic

You can use almost any ESP32 breakout variant, we tried node MCU but it proven to be not reliable, but it could have been caused by a bad quality one of.
We used the Lolin32 then.

 - You can use JST connector anything else as long as it suit your needs. You can also just solder everything to the board but it's not recommended.

Knowns issues:
 - We used 1A 12v to 5v regulator from dollar store, it proved to be unsufficient in some situation like when the servo motor move or the board starts. So 3A or more should be better.

![](./Assets/Schematic_SGU-Base.png)

The photo is old, I used JST connector later because the job is prettier.

![](./Assets/circuit-board-example.jpg)

## Program the ESP32

***Warning:*** This chapter won't explain how to program an ESP32, there are plenty of existing tutorial to explain it.

- Connect a USB cable to the base ESP32.
- Flash "ring-fw" app into it.
- Connect to "SGU-Base-XXXXXX" Wi-Fi access point without password.
- Access to http://192.168.4.1 using a browser. You should get access to the control program.