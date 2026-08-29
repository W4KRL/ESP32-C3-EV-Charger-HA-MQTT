# AC Power Monitor

## Electrical Design

The AC Power Monitor measures power consumed by an Electric Vehicle Supply Equipment (EVSE) for a home-based Level 2 charger supplied with 240 Vac. It measures root mean square (rms) values of voltage and current and computes aparrent power (volt-amps), real power (W), and power factor (W/va). The data is packaged in the MQTT format and transmitted to the Home Assistant broker by means of Wi-Fi.

The monitor is based on an ESP32-C3 SuperMini development board propgrammed in Arduino C++ using PlatformIO as the programming tool. Firmware can be updated using the on-board USB-C connector or by Arduino OTA (Over The Air).

Power is provided to the monitor from 240 Vac stepped down is a switch-mode module and regulated to 5 Vdc. Its outputs suppluies the SuperMini and the voltage sense module. The onboard regulator of the SuperMini provdes regulated 3.3 Vdc for the ESP32-C3 and current sensing circuitry.

### ESP32-C3 Analog to Digital Conversion (ADC)

The ESP32-C3 features a 12-bit SAR ADC with an effective measurable voltage range from 0 mV to approximately 2500 mV (2.5 V) depending on the input attenuation settings. It outputs raw digital values from 0 to 4095. Details are provided in the [Espressif Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/adc/index.html)

Due to non-linearity at the upper and lower ends of the range, the signal should be conditioned for 0.3 to 2.3 Vdc. The AC monitor biases the voltage and current signals to approximately the mid-point of the linear range at 1.3 Vdc. The signal ranges are designed to produce a peak deviation of 1.0 V from the dc bias (2.0 Vpeak-peak) at the monitor rating of 16 Aac and 240 Vac.

### Current Sense Network

The current sense network uses a windowed current transformer with the following characteristics:

| Item             | Parameter                            |
| ---------------- | ------------------------------------ |
| Model            | QNCT17L                              |
| Type             | Window                               |
| Outside Diameter | 26.6 mm                              |
| Inside Diameter  | 10.3 mm                              |
| Thickness        | 10.4 mm                              |
| Leads            | Red/Black 200 mm                     |
| Ratio            | 1000:1                               |
| Maximum Current  | 50 A                                 |
| Accuracy Class   | 0.2                                  |
| Manufacturer     | Nanjing Qineng Electronic Technology |
| Website          | <https://www.njqnet.com/>            |

### Burden Resistor Selection

The monitor is designed for maximum EVSE current of 16 Arms. The peak of a sine wave is the square root of 2 times the rms value:  
Apeak = $\sqrt(2)$ $\cdot$ 16 Arms = 22.6 A = ~ 23 A

Select a burden resistor to develop 1.0 V at 23 A:  
Rb = 1.0 V / 23 A = 43.5 Ohms

A standard value of 39 Ohms may be used, however, two parallel resistors may achieve a higher accuracy:  
Rb = 68 || 120 = 43.4 Ohms

The tools provided by Bjorn Jansson are very useful in calculating series and parallel resistor networks:  
<https://jansson.us/resistors.html>

### DC Bias for the Current Sense Signal
