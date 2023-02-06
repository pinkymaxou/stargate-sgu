# Settings

| Key               | Value | Min        | Max        | Type   | Description                                          |
|-------------------|-------|------------|------------|--------|------------------------------------------------------|
| Clamp.LockedPWM   | 1250  | 1000       | 2000       | int32  | Servo motor locked PWM                               |
| Clamp.ReleasPWM   | 1050  | 1000       | 2000       | int32  | Servo motor released PWM                             |
| Ring.HomeOffset   | -50   | -500       | 500        | int32  | Offset relative to home sensor                       |
| Ring.SymBright    | 15    | 3          | 50         | int32  | Symbol brightness                                    |
| StepPerRot        | 14667 | 0          | 64000      | int32  | How many step per rotation                           |
| TimePerRot        | 8537  | 0          | 120000     | int32  | Time to do a rotation                                |
| GateTimeoutS      | 300   | 10         | 2520       | int32  | Timeout (s) before the gate close                    |
| Ramp.LightOn      | 10    | 0          | 100        | int32  | Ramp illumination ON (percent)                       |
| WH.MaxBright      | 180   | 0          | 255        | int32  | Maximum brightness for wormhole leds. (Warning: can cause voltage drop) |
| WH.Type           | 1     | 0          | 2          | int32  | 0: SGU, 1: SG1, 2: Hell                              |
| WSTA.IsActive     | 1     | 0          | 1          | int32  | Wifi is active                                       |
| WSTA.SSID         | Stargate-Command |            |          | string | WiFi (SSID)                                         |
| WSTA.Pass         | uma-thurman |   |          | string | WiFi password                                       |
| Dial.anim1        | 750   | 0          | 6000       | int32  | Delay before locking the chevron (ms)                 |
| Dial.anim2        | 1250  | 0          | 6000       | int32  | Delay after locking the chevron (ms)                  |
| Dial.anim3        | 2500  | 0          | 10000      | int32  | Delay before starting to dial (ms)                   |