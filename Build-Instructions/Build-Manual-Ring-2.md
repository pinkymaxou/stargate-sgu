# Internal electronic

## Solder and assemble

- Solder the header necessary to easily program the M5Stamp Pico. The header should be accessible when the ring is disassembled.

![](./Assets/m5stamp-pico.png)

- Install batteries using hot glue. You don't need to put a lot, just tacking is good enough.

![](./Assets/battery-installation.png)

- Assemble the electronic according to this picture: (You have no obligation to position the parts this way, but ensure to balance weight as much as possible)
** Warning: ** Unfortunately there are not a lot of picture of this part. Earlier you installed a connector for WS2812B leds, you will have to soldier the other half of the connector according to the plan.

![](./Assets/interior-electronic.png)

![](./Assets/Schematic_SGU-Ring.png)

## Prime the gate

To start, the gate ring to receive power from power collector (DON'T REVERSE POLARITY!) at least to start, then the ESP32 can hold it's own power using the MOSFET. But if you power it using the programmer it will stay powered.

## Program the M5Stamp Pico

***Warning:*** This chapter won't explain how to program an ESP32, there are plenty of existing tutorial to explain it.

You need to use the ESP-IDF 5.1 toolkit to compile and program the 'ring-factory' software into the M5Stamp. The software allows to upload the real firmware over the air without using any electronical connection.

It wouldn't be very practical if it was required to dissamble the ring everytime a firmware upgrade is necessary.

The bootloader of the factory app is programmed to automatically erase the last firmware if you hold the button for 5s after startup.
You can disconnect the battery using the toggle switch, hold the button, then prime the gate.

- You need to connect the M5Stamp pico programmer to the M5Stamp pico.
- Flash "ring-factory" app into it.
- Prime the ring or let it connected to the programmer, chevron should blink with a nice pattern.
- Connect to "SGU-Ring-Fact-XXXXXX" Wi-Fi access point without password.
- To flash it with the real firmware (ring-fw), use a browser and reach this address : http://192.168.66.1 . A webpage allows you to upload a new binary directly without hassle.
