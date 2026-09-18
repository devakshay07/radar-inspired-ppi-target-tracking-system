# Pin Configuration

| Component | Pin | ESP32 GPIO | Function |
| :--- | :--- | :--- | :--- |
| HC-SR04 | TRIG | GPIO 5 | Output: Trigger ultrasonic pulse |
| HC-SR04 | ECHO | GPIO 34 | Input: Receive echo pulse (5V safe via voltage divider/input limits) |
| RCWL-0516 | OUT | GPIO 4 | Input: Microwave motion detection |
| Pan Servo | PWM | GPIO 13 | Output: Horizontal sweeping |
| Tilt Servo | PWM | GPIO 12 | Output: Vertical aiming |
