# Parts List — ESP32-C3 EV Charger AC Power Monitor

## Electronic Components

| Ref        | Description                          | Value / Spec              | Qty | Notes |
|------------|---------------------------------------|----------------------------|-----|-------|
| U1         | ZMPT101B voltage sense module          | 5 Vdc supply               | 1   | |
| U2         | ESP32-C3 SuperMini                     | —                           | 1   | |
| U3         | AC/DC converter                        | WX-DC12003, 270 Vac → 5 Vdc | 1   | |
| R1         | Resistor                               | 22 kΩ, 0.125 W              | 1   | Voltage bias divider, high side |
| R2         | Resistor                               | 10 kΩ, 0.125 W              | 1   | Voltage bias divider, low side |
| R3         | Resistor                               | 22 Ω, 0.125 W               | 1   | CT burden resistor (measured 21.7 Ω) |
| R4         | Resistor                               | 1 kΩ, 0.125 W               | 1   | |
| R5         | Resistor                               | 2 kΩ, 0.125 W               | 1   | |
| R6         | Resistor                               | 1 kΩ, 0.125 W               | 1   | |
| R7, R8     | Resistor                               | 82 kΩ, 0.5 W                | 2   | |
| C1, C2     | Capacitor, ceramic                     | 100 nF                      | 2   | ADC anti-alias filtering |
| C3         | Capacitor, electrolytic                | 10 µF, 6.3 V                | 1   | CT bias decoupling |
| CT         | Current transformer                    | 3000:1, 100 A, open-jaw     | 1   | 4 primary turns → effective 750:1 |
| —          | Stripboard, copper                     | 93 × 55 mm                  | 1   | Tayda Electronics A-5031 |
| TB1        | Terminal block, pass-through           | 2-pos, rated ≥250 Vac       | 1   | |
| TB2, TB3   | Terminal block, screw, PCB mount       | 2-pos, 0.2 in pitch         | 2   | |
| H1         | —                                       | Supplied with U1            | —   | |
| J1         | Header, female, ribbon cable           | 4-pin                       | 1   | |

## Power / Receptacle Components

| Ref | Description                              | Value / Spec         | Qty | Notes |
|-----|--------------------------------------------|------------------------|-----|-------|
| —   | Receptacle, duplex, Decora-style           | NEMA 6-20R              | 1   | |
| —   | Cord, molded plug                          | NEMA 6-20P, SJTW #12    | 1   | Length? |

## Enclosure

| Ref | Description                              | Value / Spec         | Qty | Notes |
|-----|--------------------------------------------|------------------------|-----|-------|
| —   | Enclosure box, surface mount, weatherproof, non-metallic | 2-gang | 1 | Brand/model? |
| —   | Wall plate, blank                          | —                       | 1   | Which opening — blank or does it need a cutout for anything? |
| —   | Wall plate, Decora-style opening           | —                       | 1   | |

## Still needed

- Enclosure box brand/model and part number
- Wall plate brand/model
- Cord length (NEMA 6-20P cable)
- Mounting hardware (stripboard standoffs/screws inside enclosure)
- Cable/cord strain relief or grip at enclosure entry
- Any fuse, breaker, or overcurrent protection component, if used
- Wire gauge/color for internal connections (CT leads, ZMPT leads, power)
- Fasteners for wall plate / box mounting