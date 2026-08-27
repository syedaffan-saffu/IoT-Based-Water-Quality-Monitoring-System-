# IoT-Based Water Quality Monitoring System

A basic IoT prototype for measuring water-quality indicators and viewing the readings remotely. The project uses an Arduino with pH, turbidity, and temperature sensors, while Python handles the collected data and displays it in a readable form.

## Project Objective

The goal was to build a low-cost system that can:

- Measure important water conditions in real time.
- Collect readings from multiple sensors in one place.
- Identify values that move outside simple safe thresholds.
- Make the readings easier to monitor and understand.

## Tools and Technologies

- Arduino
- pH sensor
- Turbidity sensor
- Temperature sensor
- Python
- Serial or wireless data communication
- Basic data visualization

The exact sensor models and communication module can be changed according to availability.

## Basic Methodology

1. Connected the pH, turbidity, and temperature sensors to the Arduino.
2. Read the analog or digital value produced by each sensor.
3. Applied basic calibration values to convert raw sensor output into useful readings.
4. Combined the readings into a simple data format.
5. Sent the readings to a Python program through serial or wireless communication.
6. Cleaned and normalized the received values in Python.
7. Displayed current and previous readings using simple charts.
8. Compared readings with selected thresholds and generated a warning when a value moved outside the expected range.

## System Flow

```text
Water Sensors -> Arduino -> Serial/Wireless Connection -> Python -> Charts and Alerts
```

## What I Achieved

- Collected readings from three different environmental sensors.
- Connected embedded hardware with a Python data-processing program.
- Displayed changing sensor values for remote or local monitoring.
- Added basic threshold alerts for unusual readings.
- Learned the importance of sensor calibration and handling noisy data.

## Basic Setup

1. Connect the sensors to the Arduino using the correct voltage and input pins.
2. Upload the Arduino program that reads and sends the sensor values.
3. Install the Python packages used by the visualization script.
4. Select the correct serial port or communication settings.
5. Run the Python program and place the sensors in the water sample.

Example Python dependencies may include:

```bash
pip install pyserial pandas matplotlib
```

## Limitations

- Sensor readings depend on proper calibration.
- Low-cost sensors may produce noise or drift over time.
- Threshold warnings are not a replacement for laboratory testing.
- The prototype should not be used for medical or public-safety decisions.

## Possible Improvements

- Store readings in a cloud database.
- Add a web or mobile dashboard.
- Improve calibration using known water samples.
- Add conductivity or dissolved-oxygen sensors.
- Send notifications when a threshold is crossed.

## Author

**Syed Affan Ali**  
Mobile Application Developer | Embedded Systems | IoT | Applied AI
