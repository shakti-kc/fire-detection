#include <DHT.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define dht_dpin 6
#define DHTTYPE DHT11

#define MQ_PIN A0
#define RL_VALUE 5.1
#define RO_CLEAN_AIR_FACTOR 9.83

#define CALIBARAION_SAMPLE_TIMES 50
#define CALIBRATION_SAMPLE_INTERVAL 500
#define READ_SAMPLE_INTERVAL 50
#define READ_SAMPLE_TIMES 5

#define GAS_LPG 0
#define GAS_CO 1
#define GAS_SMOKE 2

#define flameSensorPin A1
#define thresholdFlameSemsorValue 100

DHT dht(dht_dpin, DHTTYPE);
TinyGPSPlus gps;
SoftwareSerial mygps(5, 4);  // GPS Tx Pin - D4  GPS Rx Pin D3
RF24 radio(7, 8);            // CE, CSN

const byte address[6] = "00001";
float LPGCurve[3] = { 2.3, 0.21, -0.47 };
float COCurve[3] = { 2.3, 0.72, -0.34 };
float SmokeCurve[3] = { 2.3, 0.53, -0.44 };
float Ro = 10;

void setup() {
  Serial.begin(9600);

  // DHT sensor initialization
  dht.begin();

  // GPS module initialization
  mygps.begin(9600);

  // NRF24L01 initialization
  Serial.println("Initializing NRF24L01");
  if (!radio.begin()) {
    Serial.println("NRF24L01 initialization failed");
    while (1)
      ;
  }
  Serial.println("NRF24L01 initialization succeeded");
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();

  // Calibrate MQ sensor
  Serial.print("Calibrating...\n");
  Ro = MQCalibration(MQ_PIN);
  Serial.print("Calibration is done...\n");
  Serial.print("Ro=");
  Serial.print(Ro);
  Serial.print("kohm\n");
}

void loop() {
  // Read data from DHT sensor
  float tempC = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read data from MQ-2 sensor
  float lpg = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_LPG);
  float co = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
  float smoke = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE);

  // Read data from GPS module
  if (mygps.available() > 0) {
    gps.encode(mygps.read());
  }

  // Ensure GPS data is valid before using it
  double latitude = gps.location.isValid() ? gps.location.lat() : 27.687938;
  double longitude = gps.location.isValid() ? gps.location.lng() : 83.446296;

  // Read data from flame sensor
  int flameSensorValue = analogRead(flameSensorPin);
  bool fireDetected = flameSensorValue < thresholdFlameSemsorValue;
  bool smokeDetected = smoke > 100;  // You can set your own threshold for smoke

  if (fireDetected || smokeDetected) {
    // Print data before sending
    Serial.print("Temperature: ");
    Serial.println(tempC);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("LPG: ");
    Serial.println(lpg);
    Serial.print("CO: ");
    Serial.println(co);
    Serial.print("Smoke: ");
    Serial.println(smoke);
    Serial.print("Latitude: ");
    Serial.println(latitude);
    Serial.print("Longitude: ");
    Serial.println(longitude);
    Serial.print("Fire detected: ");
    Serial.println(fireDetected ? "Yes" : "No");

    String dataToSend = String(tempC) + " " + String(humidity) + " " + String(lpg) + " "
                        + String(co) + " " + String(smoke) + " " + String(latitude, 6) + " "
                        + String(longitude, 6);
    // Convert String to char array for NRF24L01
    char charBuf[dataToSend.length() + 1];
    dataToSend.toCharArray(charBuf, sizeof(charBuf));
    Serial.println(dataToSend);

    // Split data into chunks of 32 bytes and send
    int totalBytes = strlen(charBuf);
    int bytesSent = 0;
    while (bytesSent < totalBytes) {
      char tempBuf[32] = { 0 };
      int chunkSize = min(32, totalBytes - bytesSent);
      strncpy(tempBuf, charBuf + bytesSent, chunkSize);
      bool success = radio.write(&tempBuf, sizeof(tempBuf));
      if (success) {
        Serial.println("Data chunk sent successfully");
      } else {
        Serial.println("Data chunk sending failed");
      }
      bytesSent += chunkSize;
      delay(5);  // short delay between chunks
    }
  } else {
    Serial.println("No fire or smoke detected. Data not sent.");
  }
  Serial.println("===========================");

  delay(5000);  // Wait for 5 seconds before checking again
}

// MQ Sensor Functions
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

int MQGetPercentage(float rs_ro_ratio, float *pcurve) {
  return (pow(10, ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0]));
}
