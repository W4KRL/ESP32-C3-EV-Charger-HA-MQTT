# Parts List — ESP32-C3 EV Charger AC Power Monitor

## Electronic Components

| Ref    | Description                            | Value / Spec             | Qty | Notes                      |
| ------ | -------------------------------------- | ------------------------ | --- | -------------------------- |
| U1     | ZMPT101B voltage sense module          | 5 Vdc supply             | 1   |                            |
| U2     | ESP32-C3 SuperMini development board   |                          | 1   |                            |
| U3     | WX-DC12003 AC/DC converter             | 270 Vac to 5 Vdc, 700 mA | 1   |                            |
| R1     | Resistor                               | 120 kΩ, 0.5 W            | 1   | Voltage sense resistor     |
| R2     | Resistor                               | 120 kΩ, 0.5 W            | 1   | Voltage sense resistor     |
| R3     | Resistor                               | 68 Ω, 0.125 W            | 1   | CT burden resistor         |
| R4     | Resistor                               | 120 Ω, 0.125 W           | 1   | CT burden resistor         |
| R5     | Resistor                               | 3.3 kΩ, 0.125 W          | 1   | Voltage divider, low side  |
| R6     | Resistor                               | 5.1 kΩ, 0.125 W          | 1   | Voltage divider, high side |
| R7     | Resistor                               | 1 kΩ, 0.125 W            | 1   | Low pass filter            |
| R8     | Resistor                               | 2 kΩ, 0.125 W            | 1   | Voltage divider & low pass |
| R9     | Resistor                               | 2 kΩ, 0.125 W            | 1   | Voltage divider & low pass |
| R10    | Resistor                               | 150 Ω, 0.125 W           | 1   | LED dropping resistor      |
| C1, C2 | Capacitor, ceramic                     | 100 nF                   | 1   | Low pass filter            |
| C3     | Capacitor, electrolytic                | 47 uF, 16 V              | 1   | CT bias decoupling         |
| C4     | Capacitor, ceramic                     | 100 nF                   | 1   | Power supply bypass        |
| CT     | Current transformer                    | 1000:1, 50 A, window     | 1   | QNCT17L                    |
| D1     | Bicolor LED (red/green)                | 2-pin, polar changing    | 1   |                            |
| F1     | TR5 fuse                               | 250 mA, 250 Vac          | 1   |                            |
| TB1    | 2-point, screw terminal, 5.08 mm pitch | 2-pos, rated ≥250 Vac    | 1   |                            |
| H1     | 2-pin header, 0.1—in pitch             |                          | 1   |                            |
| —      | Stripboard, copper                     | 93X55 mm, 0.1-in pitch   | 1   | Tayda Electronics A-5031   |

## Power / Receptacle Components

| Ref | Description                            | Value / Spec               | Qty | Notes            |
| --- | -------------------------------------- | -------------------------- | --- | ---------------- |
| —   | Receptacle, duplex                     | NEMA 6-20R, 250 Vac, 20 A  | 1   |                  |
| —   | Extension Cord 250 Vac, 20 A, SJTW #12 | NEMA 6-20 plug/socket      | 1   | 6-ft             |
| —   | Electrical box, non-metallic, type FSE | 2-gang, 1/2-in conduit hub | 1   | CANTEX 5133380   |
| —   | Wall plate, 2-gang combo blank/duplex  | —                          | 1   | ENERLITES 880121 |
| -   | Combination connector                  | 1/2-in schedule 40         | 1   | CANTEX 5140043   |
| —   | PVC conduit                            | 1/2-in schedule 40         | 1   |                  |
| —   | Strain relief                          | 1/2-in NPT, Nylon          | 1   |                  |
