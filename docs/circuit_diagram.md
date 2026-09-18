# Circuit Diagram

```mermaid
graph TD
    %% Power
    BATT[1x 18650 Battery 3.7V] --> BOOST[CA6009 Boost Converter]
    BOOST -- 5.0V OUT --> 5V_RAIL[5V Power Rail]
    BATT -- GND --> GND_RAIL[Common Ground Rail]
    BOOST -- GND --> GND_RAIL
    
    MAC[Macbook USB] --> ESP32[ESP32 38-Pin]
    ESP32 -- GND --> GND_RAIL

    %% Sensors
    RCWL[RCWL-0516 Microwave]
    5V_RAIL --> RCWL
    GND_RAIL --> RCWL
    RCWL -- OUT 3.3V Logic --> P4[ESP32 GPIO 4]

    HCSR04[HC-SR04 Ultrasonic]
    5V_RAIL --> HCSR04
    GND_RAIL --> HCSR04
    ESP32 -- TRIG --> P5[ESP32 GPIO 5]
    HCSR04 -- ECHO 5V Logic --> P34[ESP32 GPIO 34 - Input Only]

    %% Servos
    S_PAN[Base Pan Servo]
    5V_RAIL --> S_PAN
    GND_RAIL --> S_PAN
    ESP32 -- PWM --> P13[ESP32 GPIO 13]

    S_TILT[Head Tilt Servo]
    5V_RAIL --> S_TILT
    GND_RAIL --> S_TILT
    ESP32 -- PWM --> P12[ESP32 GPIO 12]

    S_SWEEP[Radar Sweep Servo]
    5V_RAIL --> S_SWEEP
    GND_RAIL --> S_SWEEP
    ESP32 -- PWM --> P14[ESP32 GPIO 14]

    classDef power fill:#f9c,stroke:#333,stroke-width:2px;
    classDef logic fill:#9cf,stroke:#333,stroke-width:2px;
    class BATT,BOOST,5V_RAIL,GND_RAIL power;
    class ESP32,RCWL,HCSR04,S_PAN,S_TILT,S_SWEEP logic;
```

**CRITICAL NOTES:**
1. The ESP32 is powered via the Mac's USB port.
2. The Servos and Sensors are powered by the CA6009 Boost Converter.
3. **MUST DO:** You must connect the Ground (GND) of the ESP32 to the Ground (GND) of the CA6009 output. If you do not tie the grounds together, the PWM signals will float and the servos will spasm violently.
