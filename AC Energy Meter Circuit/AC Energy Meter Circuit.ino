#include <LiquidCrystal.h>
#include "EmonLib.h"

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

// Grid AC sensors
EnergyMonitor emonGrid;
const int GridCurrentPin = A1;
int gridSensitivity = 185;
int gridOffset = 2542;

// Solar AC sensors
EnergyMonitor emonSolar;
const int SolarCurrentPin = A3;
int solarSensitivity = 185;
int solarOffset = 2542;

// Industrial Load sensor
const int IndustrialPin = A4;
int industrialSensitivity = 185;
int industrialOffset = 2542;

// Residential Load sensor
const int ResidentialPin = A5;
int residentialSensitivity = 185;
int residentialOffset = 2542;

// Relay pins
const int ResGridRelay = 7;   // Residential using Grid
const int ResSolarRelay = 5;  // Residential using Solar
const int IndGridRelay = 6;   // Industrial using Grid
const int IndSolarRelay = 4;  // Industrial using Solar

// Energy accumulators
float gridEnergy = 0;
float solarEnergy = 0;
float industrialEnergy = 0;
float residentialEnergy = 0;
unsigned long lastTime = 0;

// Timing for JSON output
unsigned long lastJsonTime = 0;
const unsigned long jsonInterval = 1000; // 1 second

void setup() {
  Serial.begin(9600);

  emonGrid.voltage(A0, 187, 1.7);   // Grid voltage sensor at A0
  emonSolar.voltage(A2, 187, 1.7);  // Solar voltage sensor at A2

  lcd.begin(20, 4);

  // Relay pin setup
  pinMode(ResGridRelay, OUTPUT);
  pinMode(ResSolarRelay, OUTPUT);
  pinMode(IndGridRelay, OUTPUT);
  pinMode(IndSolarRelay, OUTPUT);

  // Initialize all relays LOW
  digitalWrite(ResGridRelay, LOW);
  digitalWrite(ResSolarRelay, LOW);
  digitalWrite(IndGridRelay, LOW);
  digitalWrite(IndSolarRelay, LOW);

  // Initial LCD status
  updateLCD();
}

void loop() {
  unsigned long currentTime = millis();
  float elapsedSec = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  // -------- RELAY CONTROL VIA SERIAL --------
  while (Serial.available() > 0) {
    char cmd = Serial.read();
    switch(cmd) {
      case 'A': digitalWrite(ResGridRelay, HIGH); Serial.println("ResGrid ON"); break;
      case 'B': digitalWrite(ResGridRelay, LOW);  Serial.println("ResGrid OFF"); break;
      case 'C': digitalWrite(ResSolarRelay, HIGH); Serial.println("ResSolar ON"); break;
      case 'D': digitalWrite(ResSolarRelay, LOW);  Serial.println("ResSolar OFF"); break;
      case 'E': digitalWrite(IndGridRelay, HIGH); Serial.println("IndGrid ON"); break;
      case 'F': digitalWrite(IndGridRelay, LOW);  Serial.println("IndGrid OFF"); break;
      case 'G': digitalWrite(IndSolarRelay, HIGH); Serial.println("IndSolar ON"); break;
      case 'H': digitalWrite(IndSolarRelay, LOW);  Serial.println("IndSolar OFF"); break;
    }
    // Update LCD after any command
    updateLCD();
  }

  // -------- SENSOR MEASUREMENTS --------
  emonGrid.calcVI(20,2000);
  float gv = emonGrid.Vrms;
  float gi = measureCurrent(GridCurrentPin, gridOffset, gridSensitivity);
  float gp = gv * gi;
  gridEnergy += (gp * elapsedSec) / 3600.0;

  emonSolar.calcVI(20,2000);
  float sv = emonSolar.Vrms;
  float si = measureCurrent(SolarCurrentPin, solarOffset, solarSensitivity);
  float sp = sv * si;
  solarEnergy += (sp * elapsedSec) / 3600.0;

  float ii = measureCurrent(IndustrialPin, industrialOffset, industrialSensitivity);
  float ip = gv * ii; 
  industrialEnergy += (ip * elapsedSec) / 3600.0;

  float ri = measureCurrent(ResidentialPin, residentialOffset, residentialSensitivity);
  float rp = gv * ri; 
  residentialEnergy += (rp * elapsedSec) / 3600.0;

  // -------- JSON OUTPUT EVERY SECOND --------
  if (currentTime - lastJsonTime >= jsonInterval) {
    lastJsonTime = currentTime;

    Serial.print("{");
    Serial.print("\"G\":{"); 
    Serial.print("\"V\":"); Serial.print(gv,1); Serial.print(",");
    Serial.print("\"I\":"); Serial.print(gi,2); Serial.print(",");
    Serial.print("\"P\":"); Serial.print(gp,1); Serial.print(",");
    Serial.print("\"E\":"); Serial.print(gridEnergy,1);
    Serial.print("},");

    Serial.print("\"S\":{"); 
    Serial.print("\"V\":"); Serial.print(sv,1); Serial.print(",");
    Serial.print("\"I\":"); Serial.print(si,2); Serial.print(",");
    Serial.print("\"P\":"); Serial.print(sp,1); Serial.print(",");
    Serial.print("\"E\":"); Serial.print(solarEnergy,1);
    Serial.print("},");

    Serial.print("\"Ind\":{"); 
    Serial.print("\"I\":"); Serial.print(ii,2); Serial.print(",");
    Serial.print("\"P\":"); Serial.print(ip,1); Serial.print(",");
    Serial.print("\"E\":"); Serial.print(industrialEnergy,1);
    Serial.print("},");

    Serial.print("\"Res\":{"); 
    Serial.print("\"I\":"); Serial.print(ri,2); Serial.print(",");
    Serial.print("\"P\":"); Serial.print(rp,1); Serial.print(",");
    Serial.print("\"E\":"); Serial.print(residentialEnergy,1);
    Serial.print("}");

    Serial.println("}");
  }
}

// -------- Helper Function --------
float measureCurrent(int pin, int offset, int sensitivity) {
  unsigned int temp = 0;
  float maxpoint = 0;
  for(int i=0;i<500;i++) {
    temp = analogRead(pin);
    if(temp > maxpoint) {
      maxpoint = temp;
    }
  }
  double eVoltage = (maxpoint / 1024.0) * 5000; // mV
  double Current = (eVoltage - offset) / sensitivity;
  return Current / sqrt(2); // RMS
}

// -------- LCD Update Function --------
void updateLCD() {
  lcd.setCursor(0,0);
  lcd.print("ResGrid: ");
  lcd.print(digitalRead(ResGridRelay) ? "ON " : "OFF");

  lcd.setCursor(0,1);
  lcd.print("ResSolar: ");
  lcd.print(digitalRead(ResSolarRelay) ? "ON " : "OFF");

  lcd.setCursor(0,2);
  lcd.print("IndGrid: ");
  lcd.print(digitalRead(IndGridRelay) ? "ON " : "OFF");

  lcd.setCursor(0,3);
  lcd.print("IndSolar: ");
  lcd.print(digitalRead(IndSolarRelay) ? "ON " : "OFF");
}
