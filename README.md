### Project FLAiRE
## Fire Location Alert and Intelligent Response Engine
FLAiRE is an advanced fire safety system that automatically detects smoke and flames, activates a sprinkler system, and sends an SMS alert to the nearest fire station. This system is designed to ensure rapid response to fire incidents, minimizing damage and improving emergency reaction time.

## ✨ Features
# 🔥 Intelligent Fire Detection
FLAiRE uses flame sensors and smoke detectors to monitor fire threats in real-time with high sensitivity, reducing the chances of false positives.

# 💧 Automated Sprinkler System
Once a fire is detected, FLAiRE immediately activates the sprinkler system connected to a relay module, helping suppress the fire before it spreads.

# 📱 Instant SMS Alert
FLAiRE uses a GSM module (e.g., SIM800L) to send an SMS alert to the nearest fire station or emergency contacts, ensuring prompt response.

# 🌡️ Environmental Monitoring 
You can add optional temperature and gas sensors to enhance fire detection reliability and further reduce false alarms.

# 🛠️ Components
Wemos D1: The microcontroller responsible for managing the system.

Flame Sensor: Detects the presence of flames.

Smoke Sensor (MQ-2 or MQ-135): Monitors smoke levels to identify fire threats.

Relay Module: Controls the water pump or solenoid valve to activate the sprinkler system.

Water Pump / Solenoid Valve: Activates the sprinkler for fire suppression.

GSM Module (e.g., SIM800L): Sends automated SMS alerts to the fire station or emergency contacts.

LEDs & Resistors: Visual indicators for system status.

Buzzer: Alerts users with an audible sound when fire or system errors are detected.

Power Supply (5V–12V): Powers the system components.

## 🖊️ Planned Features
# 🗺️ Location-Based Fire Station Lookup
Integration with a GPS module will allow the system to send the exact location of the fire to the nearest fire station.

# 📶 IoT Dashboard
The system will include an IoT dashboard for real-time monitoring and event logging, accessible via a web or mobile app.

# ⚙️ Manual Override & Reset
Push buttons will provide manual control for system testing, sprinkler activation, and resetting the system.
