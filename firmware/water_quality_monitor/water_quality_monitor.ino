#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BlynkSimpleEsp32.h>
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* auth = "YOUR_BLYNK_AUTH_TOKEN";


#define SensorPin 39
#define ONE_WIRE_BUS 13
#define EC_Pin 34
#define ORP_PIN 32
#define DO_PIN 33
#define TDS_SENSOR_PIN 36
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define turbidityPin 35
#define VREF 3300.0
#define ADC_RESOLUTION 4095.0

int buf[10]; // Buffer array for pH sensor readings
int temp; // Temporary variable used in sorting
int avgValue; // Average value for pH sensor readings

// Define weights
const float weightTemperature = 0.10;
const float weightPH = 0.15;
const float weightTDS = 0.17;
const float weightTurbidity = 0.15;
const float weightORP = 0.12;
const float weightEC = 0.18;
const float weightDO = 0.15;

// DO specific definitions and variables
#define VREF_DO 3300    // VREF (mv)
#define ADC_RES_DO 4096 // ADC Resolution

// Single-point calibration Mode=0
// Two-point calibration Mode=1
#define TWO_POINT_CALIBRATION 0

#define READ_TEMP (32)
// Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (791) // mv
#define CAL1_T (32)  // ℃
// Two-point calibration needs to be filled CAL2_V and CAL2_T
// CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1300) // mv
#define CAL2_T (15)   // ℃

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
float DO;
float DO1;

const int ArrayLength = 40; // Length of the ORP readings array
int orpArray[ArrayLength]; // Array to store ORP readings
int orpArrayIndex = 0; // Index for ORP readings array
const float VOLTAGE = 3.3; // Reference voltage for ORP calculation
const float OFFSET = -397; // Offset value for ORP calculation (adjust as needed)

float orpValue = 0; // Declare orpValue as a global variable

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, password);
  sensors.begin();
}

void loop() {
  Blynk.run();
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println(" °C");
  Blynk.virtualWrite(V0, tempC);

  // // Array of specific temperature values
  // float tempValues[] = {28.92, 28.99, 29.07, 29.12, 29.20};
  // // Generate a random index to pick one of the values
  // int index = random(0, 5);
  // // Get the randomly picked temperature value
  // float tempC = tempValues[index];
  
  // Serial.print("Temperature: ");
  // Serial.print(tempC);
  // Serial.println(" °C");
  // Blynk.virtualWrite(V0, tempC);


  int turbidityValue = analogRead(turbidityPin);
  float voltageTurbidity = turbidityValue * (VREF / ADC_RESOLUTION);
  float turbidity = convertToTurbidity(voltageTurbidity);
  Serial.print("Turbidity: ");
  Serial.print(turbidity, 2);
  Serial.println(" NTU");
  Blynk.virtualWrite(V1, turbidity);

  for (int i = 0; i < 10; i++) {
    buf[i] = analogRead(SensorPin);
    delay(10);
  }

  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  avgValue = 0;
  for (int i = 2; i < 8; i++) {
    avgValue += buf[i];
  }
  avgValue /= 6;
  float voltage = (float)avgValue * 3.3 / 4096;
  float mv = voltage * 1000.0;
  float a = (9.18 - 4.00) / (2615 - 659);
  float b = 4.00 - a * 659;
  float phValue = a * mv + b;

  Serial.print("pH: ");
  Serial.print(phValue, 2);
  Serial.println(" ");
  Blynk.virtualWrite(V2, phValue);

  float voltageEC = analogRead(EC_Pin);
  float EC = voltageEC / 49.5 / 4.35;  
  Serial.print("Conductivity: ");
  Serial.print(EC, 4);  
  Serial.println(" ms/cm");
  Blynk.virtualWrite(V3, EC);

  static unsigned long orpTimer = millis();
  if (millis() >= orpTimer) {
    orpTimer = millis() + 20;
    int rawAnalogValue = analogRead(ORP_PIN);    
    orpArray[orpArrayIndex++] = rawAnalogValue;
    if (orpArrayIndex == ArrayLength) {
      orpArrayIndex = 0;
    }
    double averageAnalog = averageArray(orpArray, ArrayLength);
    double sensorMillivolts = averageAnalog * (VOLTAGE * 1000.0 / 4096.0);  
    orpValue = ((30 * VOLTAGE * 1000) - (75 * averageAnalog * VOLTAGE * 1000 / 4096)) / 75 - OFFSET ;
    Serial.print("ORP: ");
    Serial.print(orpValue);
    Serial.println(" mV");
    Blynk.virtualWrite(V4, orpValue);
  }


  // DO specific code
  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = ((uint32_t)VREF_DO * ADC_Raw / ADC_RES_DO);

  // Print raw ADC value and calculated voltage
  Serial.print("Raw DO ADC Value: ");
  Serial.println(ADC_Raw);
  Serial.print("Calculated DO Voltage: ");
  Serial.println(ADC_Voltage);

  DO = readDO(ADC_Voltage, Temperaturet);
  DO1 = DO / 1000;
  
  // Print the intermediate DO value
  Serial.print("Raw DO: ");
  Serial.println(DO);
  Serial.print("DO (mg/L): ");
  Serial.println(DO1, 2);
  Blynk.virtualWrite(V5, DO1);

  int analogValue = analogRead(TDS_SENSOR_PIN);
  float voltageTDS = analogValue * (VREF / ADC_RESOLUTION); // Convert to mV
  float tdsValue = convertToTDS(voltageTDS);
  Serial.print("TDS Value: ");
  Serial.print(tdsValue, 2);
  Serial.println(" ppm");
  Blynk.virtualWrite(V6, tdsValue);

  
  // Calculate subindices
  int subindexTemperature = calculateSubindexTemperature(tempC);
  int subindexPH = calculateSubindexPH(phValue);
  int subindexTDS = calculateSubindexTDS(tdsValue);
  int subindexTurbidity = calculateSubindexTurbidity(turbidity);
  int subindexORP = calculateSubindexORP(orpValue);
  int subindexEC = calculateSubindexEC(EC);
  int subindexDO = calculateSubindexDO(DO1); // Use DO1 for subindex calculation

  // Calculate overall water quality index
  float waterQualityIndex = 
      subindexTemperature * weightTemperature +
      subindexPH * weightPH +
      subindexTDS * weightTDS +
      subindexTurbidity * weightTurbidity +
      subindexORP * weightORP +
      subindexEC * weightEC +
      subindexDO * weightDO;

  // Print the water quality index
  Serial.print("Water Quality Index: ");
  Serial.println(waterQualityIndex);
  Blynk.virtualWrite(V7, waterQualityIndex);

  delay(500); // Send readings every half of second
}

float readDO(uint16_t voltage_mv, uint8_t temperature) {
#if TWO_POINT_CALIBRATION == 0
  uint32_t DO_Table_Temp = pgm_read_word_near(&DO_Table[temperature]);
  float DO_mg_L = (voltage_mv * DO_Table_Temp) / CAL1_V;
  return DO_mg_L;
#else
  uint32_t DO_Value_Temp = pgm_read_word_near(&DO_Table[temperature]);
  float K = (float)(CAL1_V - CAL2_V) / (float)(CAL1_T - CAL2_T);
  uint16_t V_saturation = CAL1_V + K * (temperature - CAL1_T);
  float DO_mg_L = (float)(voltage_mv * DO_Value_Temp) / V_saturation;
  return DO_mg_L;
#endif
}

float averageArray(int* arr, int number) {
  int i;
  int max, min;
  float avg;
  long amount = 0;
  if (number <= 0) {
    Serial.println("Error number for the array to averaging!/n");
    return 0;
  }
  if (number < 5) { //less than 5, calculated directly statistics
    for (i = 0; i < number; i++) {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  } else {
    if (arr[0] < arr[1]) {
      min = arr[0];
      max = arr[1];
    } else {
      min = arr[1];
      max = arr[0];
    }
    for (i = 2; i < number; i++) {
      if (arr[i] < min) {
        amount += min; //arr<min
        min = arr[i];
      } else {
        if (arr[i] > max) {
          amount += max; //arr>max
          max = arr[i];
        } else {
          amount += arr[i]; //min<=arr<=max
        }
      }
    }
    avg = (float)amount / (number - 2);
  }
  return avg;
}

float convertToTurbidity(float voltage) {
  float voltage1 = 0.0; // Voltage when sensor is completely blocked
  float turbidity1 = 66.0; // NTU when sensor is completely blocked

  float voltage2 = 2340.0; // Voltage when sensor is in tap water
  float turbidity2 = 2.20; // NTU for tap water

  float turbidity = turbidity1 + (voltage - voltage1) * (turbidity2 - turbidity1) / (voltage2 - voltage1);

  return turbidity;
}


float convertToTDS(float voltage) {
  float voltage1 = 987.0; // Voltage of sample one in mV
  float tds1 = 334.0; // Actual TDS value of sample one in ppm

  float voltage2 = 1990.0; // Voltage of sample two in mV
  float tds2 = 857.0; // Actual TDS value of sample two in ppm

  // Linear interpolation formula
  float tdsValue = tds1 + (voltage - voltage1) * (tds2 - tds1) / (voltage2 - voltage1);

  return tdsValue;
}

int calculateSubindexTemperature(float tempC) {
  if (tempC >= 20 && tempC <= 25) return 100;
  if ((tempC >= 15 && tempC < 20) || (tempC > 25 && tempC <= 30)) return 85;
  if ((tempC >= 10 && tempC < 15) || (tempC > 30 && tempC <= 35)) return 65;
  if ((tempC >= 5 and tempC < 10) || (tempC > 35 and tempC <= 40)) return 45;
  if ((tempC >= 0 and tempC < 5) || (tempC > 40 and tempC <= 45)) return 25;
  return 5;
}
// int calculateSubindexTemperature(float tempC) {
//   if (tempC >= 20.0 && tempC <= 25.0) return 100;
//   else if (tempC >= 18.0 && tempC < 20.0) return 90;
//   else if (tempC > 25.0 && tempC <= 28.0) return 80;
//   else if (tempC >= 16.0 && tempC < 18.0) return 70;
//   else if (tempC > 28.0 && tempC <= 30.0) return 60;
//   else if (tempC >= 14.0 && tempC < 16.0) return 50;
//   else if (tempC > 30.0 && tempC <= 32.0) return 40;
//   else if (tempC >= 10.0 && tempC < 14.0) return 30;
//   else if (tempC > 32.0 && tempC <= 34.0) return 20;
//   else return 10;
// }

int calculateSubindexPH(float phValue) {
  if (phValue >= 6.5 && phValue <= 8.5) return 100;
  else if (phValue >= 6.0 && phValue < 6.5) return 90;
  else if (phValue > 8.5 && phValue <= 9.0) return 80;
  else if (phValue >= 5.5 && phValue < 6.0) return 70;
  else if (phValue > 9.0 && phValue <= 9.5) return 60;
  else if (phValue >= 5.0 && phValue < 5.5) return 50;
  else if (phValue > 9.5 && phValue <= 10.0) return 40;
  else if (phValue >= 4.5 && phValue < 5.0) return 30;
  else if (phValue > 10.0 && phValue <= 10.5) return 20;
  else return 10;
}

int calculateSubindexTDS(float tdsValue) {
  
  if (tdsValue > 90 && tdsValue <= 300) return 100;
  else if (tdsValue > 300 && tdsValue <= 500) return 90;
  else if (tdsValue > 500 && tdsValue <= 700) return 70;
  else if (tdsValue > 700 && tdsValue <= 1000) return 50;
  else if (tdsValue > 1000 && tdsValue <= 1500) return 40;
  else if (tdsValue > 1500 && tdsValue <= 2000) return 30;
  else if (tdsValue > 2000 && tdsValue <= 3000) return 20;
  else return 0;
}

// int calculateSubindexTurbidity(float turbidity) {
//   if (turbidity <= 1) return 100;
//   else if (turbidity > 1 && turbidity <= 2) return 90;
//   else if (turbidity > 2 && turbidity <= 3) return 80;
//   else if (turbidity > 3 && turbidity <= 5) return 70;
//   else if (turbidity > 5 && turbidity <= 10) return 60;
//   else if (turbidity > 10 && turbidity <= 15) return 50;
//   else if (turbidity > 15 && turbidity <= 20) return 40;
//   else if (turbidity > 20 && turbidity <= 25) return 30;
//   else if (turbidity > 25 && turbidity <= 30) return 20;
//   else return 10;
// }
int calculateSubindexTurbidity(float turbidity) {
  if (turbidity <= 1) return 100;
  if (turbidity > 1 and turbidity <= 2) return 85;
  if (turbidity > 2 and turbidity <= 3) return 75;
  if (turbidity > 3 and turbidity <= 5) return 65;
  if (turbidity > 5 and turbidity <= 10) return 45;
  if (turbidity > 10 and turbidity <= 25) return 25;
  return 5;
}

// int calculateSubindexORP(float orpValue) {
//   if (orpValue >= 700) return 100;
//   else if (orpValue >= 600 && orpValue < 700) return 90;
//   else if (orpValue >= 500 && orpValue < 600) return 80;
//   else if (orpValue >= 400 && orpValue < 500) return 70;
//   else if (orpValue >= 300 && orpValue < 400) return 60;
//   else if (orpValue >= 200 && orpValue < 300) return 50;
//   else if (orpValue >= 100 && orpValue < 200) return 40;
//   else if (orpValue >= 0 && orpValue < 100) return 30;
//   else if (orpValue >= -100 && orpValue < 0) return 20;
//   else return 10;
// }

int calculateSubindexORP(float orpValue) {
  if (orpValue >= 400 and orpValue <= 600) return 100;
  if (orpValue >= 601 and orpValue < 800) return 90;
  if (orpValue >= 801 and orpValue < 1000) return 65;
  if (orpValue >= 1001 and orpValue < 1200)  return 45;
  if (orpValue >= 1201 and orpValue < 1500) return 25;
  if (orpValue > 200 and orpValue <= 400) return 80;
 return 0;
}


// int calculateSubindexEC(float ecValue) {
//   if (ecValue <= 0.2) return 100;
//   else if (ecValue > 0.2 && ecValue <= 0.4) return 90;
//   else if (ecValue > 0.4 && ecValue <= 0.6) return 80;
//   else if (ecValue > 0.6 && ecValue <= 0.8) return 70;
//   else if (ecValue > 0.8 && ecValue <= 1.0) return 60;
//   else if (ecValue > 1.0 && ecValue <= 1.5) return 50;
//   else if (ecValue > 1.5 && ecValue <= 2.0) return 40;
//   else if (ecValue > 2.0 && ecValue <= 2.5) return 30;
//   else if (ecValue > 2.5 && ecValue <= 3.0) return 20;
//   else return 10;
// }


int calculateSubindexEC(float EC) {
  if (EC >= 0.0001 and EC <= 0.4) return 100;
  if (EC > 0.41 and EC <= 0.6) return 85;
  if (EC > 0.61 and EC <= 0.9) return 65;
  if (EC > 0.91 and EC <= 1.5) return 50;
  if (EC > 1.51 and EC <= 3.0) return 25;
  return 0;
}


// int calculateSubindexDO(float DO1) {
//   if (DO1 >= 7.0) return 100;
//   else if (DO1 >= 6.0 && DO1 < 7.0) return 90;
//   else if (DO1 >= 5.0 && DO1 < 6.0) return 80;
//   else if (DO1 >= 4.0 && DO1 < 5.0) return 70;
//   else if (DO1 >= 3.0 && DO1 < 4.0) return 60;
//   else if (DO1 >= 2.0 && DO1 < 3.0) return 50;
//   else if (DO1 >= 1.0 && DO1 < 2.0) return 40;
//   else if (DO1 >= 0.5 && DO1 < 1.0) return 30;
//   else if (DO1 >= 0.2 && DO1 < 0.5) return 20;
//   else return 10;
// }

int calculateSubindexDO(float do1) {
  if (do1 >= 6 and do1 <= 9) return 100;
  if ((do1 >= 5 and do1 < 6) || (do1 > 9 and do1 <= 10)) return 85;
  if ((do1 >= 4 and do1 < 5) || (do1 > 10 and do1 <= 11)) return 65;
  if ((do1 >= 3 and do1 < 4) || (do1 > 11 and do1 <= 12)) return 45;
  if ((do1 >= 2 and do1 < 3) || (do1 > 12 and do1 <= 13)) return 25;
  return 0;
}

