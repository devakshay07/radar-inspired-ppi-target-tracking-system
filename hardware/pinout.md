# Pin Configuration

| Component | Pin | ESP32 GPIO | Function |
| :--- | :--- | :--- | :--- |
| HC-SR04 | TRIG | GPIO 5 | Output: Trigger ultrasonic pulse |
| HC-SR04 | ECHO | GPIO 34 | Input: Receive echo pulse |
| RCWL-0516 | OUT | GPIO 4 | Input: Microwave motion detection |
| Scan Servo | PWM | GPIO 26 | Output: Radar scanning sweeps (Safe Pin) |
| Pan Servo | PWM | GPIO 25 | Output: Turret horizontal targeting (Safe Pin) |
| Tilt Servo | PWM | GPIO 27 | Output: Turret vertical aiming (Safe Pin) |
