// Required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <math.h>
#include <SoftwareSerial.h>

// Pin definitions
#define MQ2_PIN A0      // MQ2 sensor connected to analog pin A0
#define RELAY_PIN D5    // Relay connected to digital pin D5
#define BUZZER_PIN D6   // Buzzer connected to digital pin D6

// WiFi credentials
const char* ssid = "Capstone";
const char* password = "helpmelord";

// Threshold for CO detection 
const int CO_THRESHOLD = 250; 

// GSM Module Pins for SIM800L communication
#define SIM800_TX_PIN D7
#define SIM800_RX_PIN D8

// Create a SoftwareSerial object for GSM communication
SoftwareSerial sim800(SIM800_TX_PIN, SIM800_RX_PIN);

// Web server instance
ESP8266WebServer server(80);

// Constants for MQ sensor calibration
#define RL_VALUE 10.0    // Load resistor value (in kΩ)
#define RO_CLEAN_AIR_FACTOR 9.83 // For calibration
#define CALIBARAION_SAMPLE_TIMES 5
#define CALIBRATION_SAMPLE_INTERVAL 1000
#define READ_SAMPLE_TIMES 5
#define READ_SAMPLE_INTERVAL 1000

/**********************Application Related Macros**********************************/
#define GAS_LPG (0)
#define GAS_CO (1)
#define GAS_SMOKE (2)

// Function prototypes
float LPGCurve[3] = {2.3, 0.21, -0.47};  // Two points from the curve for LPG
float COCurve[3] = {2.3, 0.72, -0.34};  // Two points from the curve for CO
float SmokeCurve[3] = {2.3, 0.53, -0.44};  // Two points from the curve for smoke
float MQResistanceCalculation(int raw_adc);
float MQCalibration(int mq_pin);
float MQRead(int mq_pin);
float readVoltage(int analogValue);
int MQGetGasPercentage(float rs_ro_ratio, int gas_id);
int MQGetPercentage(float rs_ro_ratio, float* pcurve);
int calculatePPM(int analogValue, int maxPPM);
void handleRoot();
void handleLPG();
void handleCO();
void handleSmoke();
void handleAlarmState();
void sendSmsAlert(float coLevel, bool &alertSent);

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);

  // Initialize the pins
  pinMode(MQ2_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Ensure relay and buzzer are off initially
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize GSM communication
  sim800.begin(9600);
  Serial.println("GSM Module Initialized");
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/readLPG", handleLPG);
  server.on("/readCO", handleCO);
  server.on("/readSmoke", handleSmoke);
  server.on("/alarmState", handleAlarmState);

  server.begin();
  Serial.println("Web server started");
}

bool alertSent = false; // Flag to prevent duplicate alerts

void loop() {
  int sensorValue = analogRead(MQ2_PIN);
  int co = calculatePPM(sensorValue, 1000);

  if (co > CO_THRESHOLD) {
    // Activate relay and buzzer
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Danger! CO level above threshold. Sprinkler and buzzer activated.");

    // Send SMS alert if not already sent
    sendSmsAlert(co, alertSent);
  } else {
    // Deactivate relay and buzzer
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("CO level normal. System in standby.");

    // Reset the alert flag when CO levels are back to normal
    alertSent = false;
  }

  // Handle web server requests
  server.handleClient();

  // Wait for a short interval before the next reading
  delay(2000);
}

// MQ functions
float MQResistanceCalculation(int raw_adc) {
  return ((float)RL_VALUE * (1023 - raw_adc) / raw_adc);
}

float MQCalibration(int mq_pin) {
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES;
  val = val / RO_CLEAN_AIR_FACTOR;

  return val;
}

float MQRead(int mq_pin) {
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  if (gas_id == GAS_LPG) {
    return MQGetPercentage(rs_ro_ratio, LPGCurve);
  } else if (gas_id == GAS_CO) {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  } else if (gas_id == GAS_SMOKE) {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }
  return 0;
}

int MQGetPercentage(float rs_ro_ratio, float* pcurve) {
  return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

// Helper function to convert analog value to voltage
float readVoltage(int analogValue) {
  return analogValue * (5 / 1023.0); // 
}

// Helper function to calculate PPM from analog value
int calculatePPM(int analogValue, int maxPPM) {
  return map(analogValue, 0, 1023, 0, maxPPM);
}

// Handle root route
void handleRoot() {
  int sensorValue = analogRead(MQ2_PIN);
  float voltage = readVoltage(sensorValue);
  int lpg = calculatePPM(sensorValue, 1000);
  int co = calculatePPM(sensorValue, 1000);
  int smoke = calculatePPM(sensorValue, 1000);

  // Determine the state of the buzzer and sprinkler
  String buzzerState = (co > CO_THRESHOLD) ? "ACTIVE" : "INACTIVE";
  String sprinklerState = (co > CO_THRESHOLD) ? "ACTIVE" : "INACTIVE";

  String html = "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head><title>Air Quality Monitoring and Fire Alarm System</title></head>\n";
  html += "<body>\n";
  html += "<h1>Air Quality Monitoring System</h1>\n";
  html += "<p>Voltage: " + String(voltage, 2) + " V</p>\n";
  html += "<p>LPG Level: " + String(lpg) + " ppm</p>\n";
  html += "<p>CO Level: " + String(co) + " ppm</p>\n";
  html += "<p>Smoke Level: " + String(smoke) + " ppm</p>\n";

  if (co > CO_THRESHOLD) {
    html += "<p style='color:red;'>Danger! CO level above threshold.</p>\n";
  } else {
    html += "<p style='color:green;'>CO level is normal.</p>\n";
  }

  html += "<h2>Fire Alarm System</h2>\n";
  html += "<p>Alarm State: <b>" + String(buzzerState) + "</b></p>\n";
  html += "<p>Sprinkler State: <b>" + String(sprinklerState) + "</b></p>\n";

  html += "</body>\n";
  html += "</html>\n";

  server.send(200, "text/html", html);
}

// Handle LPG reading
void handleLPG() {
  int sensorValue = analogRead(MQ2_PIN);
  int lpg = calculatePPM(sensorValue, 1000);
  server.send(200, "text/plain", String(lpg));
}

// Handle CO reading
void handleCO() {
  int sensorValue = analogRead(MQ2_PIN);
  int co = calculatePPM(sensorValue, 1000);
  server.send(200, "text/plain", String(co));
}

// Handle smoke reading
void handleSmoke() {
  int sensorValue = analogRead(MQ2_PIN);
  int smoke = calculatePPM(sensorValue, 1000);
  server.send(200, "text/plain", String(smoke));
}

// Handle alarm state
void handleAlarmState() {
  int sensorValue = analogRead(MQ2_PIN);
  int co = calculatePPM(sensorValue, 1000);
  String state = (co > CO_THRESHOLD) ? "ACTIVE" : "INACTIVE";
  server.send(200, "text/plain", state);
}

void sendSmsAlert(float coLevel, bool &alertSent) {
  // Check if alert was already sent
  if (alertSent) {
    return; // Exit if already sent
  }

  // Get the current time (in milliseconds since the system started)
  unsigned long currentTime = millis();
  int hours = (currentTime / 3600000) % 24; // Get the hours in 24-hour format
  int minutes = (currentTime / 60000) % 60; // Get the minutes
  int seconds = (currentTime / 1000) % 60; // Get the seconds

  // Format the time to hh:mm:ss with leading zeros
  String timeString = (hours < 10 ? "0" : "") + String(hours) + ":" +
                      (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                      (seconds < 10 ? "0" : "") + String(seconds);

  // The location can be static or dynamic based on your setup
  String location = "Limay Senior High School, Limay, Bataan";

  // Creating the fire alert message
  String alertMessage = "FIRE ALERT! Emergency at: " + location + "\n";
  alertMessage += "CO Level: " + String(coLevel) + " ppm\n";
  alertMessage += "Time of alert: " + timeString + "\n";
  alertMessage += "Please dispatch assistance immediately.";

  Serial.println("Sending FIRE ALERT...");

  // Set GSM Module to Text Mode
  sim800.println("AT+CMGF=1");
  delay(1000); // Wait for 1 second

  // Send the SMS to the fire station number (replace with the actual fire station's number)
  sim800.println("AT+CMGS=\"+639700308993\"\r");
  delay(1000); // Wait for 1 second

  // Send the fire alert message
  sim800.println(alertMessage);
  delay(100); // Wait for a moment to allow the message to be processed

  // Send "CTRL+Z" to indicate the end of the message
  sim800.println((char)26);
  delay(1000); // Wait for 1 second

  // Mark alert as sent
  alertSent = true;

  Serial.println("Fire alert SMS sent.");
}