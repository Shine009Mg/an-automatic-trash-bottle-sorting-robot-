#include <WiFi.h>
#include <PubSubClient.h>

#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

//////////////////////////////////////////////////////
// WIFI + MQTT
//////////////////////////////////////////////////////

const char* ssid = "Shine";
const char* password = "shine123";

const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

//////////////////////////////////////////////////////
// SERVO
//////////////////////////////////////////////////////

#define Servo_Base 12
#define Servo_Top 13

Servo Servo1;
Servo Servo2;

bool isRunning = true;

//////////////////////////////////////////////////////
// HX711
//////////////////////////////////////////////////////

#define DT1 4
#define SCK1 16

#define DT2 19
#define SCK2 5

HX711 scale1;

float calibration_factor1 = 1700.0;

//////////////////////////////////////////////////////
// SENSORS
//////////////////////////////////////////////////////

#define Inductive_Pin 21

#define BUZZER 23
#define LED_RED 2

// ultrasonic 1
#define TRIG1 15
#define ECHO1 17

// hall sensors
#define HALL1_pin 27
#define HALL2_pin 32
#define HALL3_pin 33

// ultrasonic 2
#define TRIG2 25
#define ECHO2 34

// ultrasonic 3
#define TRIG3 26
#define ECHO3 35

//////////////////////////////////////////////////////
// LCD
//////////////////////////////////////////////////////

LiquidCrystal_I2C lcd(0x27, 16, 2);

//////////////////////////////////////////////////////
// VARIABLES
//////////////////////////////////////////////////////

bool hasBeeped = false;
bool needTare = false;

int count_metal = 0;
int count_plastic = 0;
int count_glass = 0;

//////////////////////////////////////////////////////
// WIFI FUNCTION
//////////////////////////////////////////////////////

void setup_wifi() {

  Serial.println();
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

//////////////////////////////////////////////////////
// MQTT RECONNECT
//////////////////////////////////////////////////////

void reconnect() {

  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP32SmartBin-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println("connected");

    } else {

      Serial.print("failed, rc=");
      Serial.println(client.state());

      delay(2000);
    }
  }
}

//////////////////////////////////////////////////////
// BUZZER
//////////////////////////////////////////////////////

void beep(int duration) {

  unsigned long endTime = millis() + duration;

  while (millis() < endTime) {

    digitalWrite(BUZZER, HIGH);
    delayMicroseconds(500);

    digitalWrite(BUZZER, LOW);
    delayMicroseconds(500);
  }

  delay(50);
}

void shortBeep() {
  beep(100);
}

void longBeep() {
  beep(500);
}

void doubleBeep() {
  beep(100);
  beep(100);
}

void alarmBeep() {
  beep(500);
  beep(100);
  beep(500);
}

//////////////////////////////////////////////////////
// ULTRASONIC
//////////////////////////////////////////////////////

float getDistance(int trigPin, int echoPin) {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  return duration * 0.034 / 2;
}

float getDistanceAvg(int trigPin, int echoPin) {

  float sum = 0;
  int validCount = 0;

  for (int i = 0; i < 5; i++) {

    float d = getDistance(trigPin, echoPin);

    if (d > 0) {
      sum += d;
      validCount++;
    }


    delay(50);
  }

  if (validCount == 0) return 0;

  return sum / validCount;
}

//////////////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  //////////////////////////////////////////////////////
  // WIFI + MQTT
  //////////////////////////////////////////////////////

  setup_wifi();

  client.setServer(mqtt_server, 1883);

  //////////////////////////////////////////////////////
  // LCD
  //////////////////////////////////////////////////////

  Wire.begin(14, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("System Start ");

  //////////////////////////////////////////////////////
  // PIN MODES
  //////////////////////////////////////////////////////

  pinMode(HALL1_pin, INPUT);
  pinMode(HALL2_pin, INPUT);
  pinMode(HALL3_pin, INPUT);

  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);

  pinMode(BUZZER, OUTPUT);

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);

  pinMode(Inductive_Pin, INPUT);

  //////////////////////////////////////////////////////
  // SERVO
  //////////////////////////////////////////////////////

  Servo1.setPeriodHertz(50);
  Servo2.setPeriodHertz(50);

  Servo1.attach(Servo_Base, 500, 2500);
  Servo2.attach(Servo_Top, 500, 2500);

  Servo2.write(32);

  //////////////////////////////////////////////////////
  // HOMING
  //////////////////////////////////////////////////////

  Serial.println("Homing Servo1 to Plastic position...");

  Servo1.write(160);

  unsigned long homingStart = millis();

  while (digitalRead(HALL2_pin) != LOW) {

    if (millis() - homingStart > 5000) {

      Serial.println("Homing timeout! Check Plastic Hall Sensor.");
      break;
    }

    delay(10);
  }

  Servo1.write(90);

  Serial.println("Servo1 Homing Done!");

  //////////////////////////////////////////////////////
  // HX711
  //////////////////////////////////////////////////////

  digitalWrite(TRIG1, LOW);
  digitalWrite(TRIG2, LOW);
  digitalWrite(TRIG3, LOW);

  scale1.begin(DT1, SCK1);

  scale1.set_scale(calibration_factor1);

  scale1.tare();

  //////////////////////////////////////////////////////
  // READY
  //////////////////////////////////////////////////////

  Serial.println("System Ready (1 Load Cell - Multiplied by 2)");

  delay(500);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SMART BIN READY");

  doubleBeep();
}

//////////////////////////////////////////////////////
// LOOP
//////////////////////////////////////////////////////

void loop() {
  float d1 = getDistanceAvg(TRIG1, ECHO1);
  Serial.println(d1);
  float d2 = getDistanceAvg(TRIG2, ECHO2);
  Serial.println(d2);
  float d3 = getDistanceAvg(TRIG3, ECHO3);
  Serial.println(d3);
  //////////////////////////////////////////////////////
  // MQTT
  //////////////////////////////////////////////////////

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  //////////////////////////////////////////////////////
  // WEIGHT
  //////////////////////////////////////////////////////

  float weight1 = scale1.get_units(10);

  float totalWeight = weight1 * 2.0;

  Serial.print("W1: ");
  Serial.print(weight1, 2);

  Serial.print(" g | Total (W1*2): ");
  Serial.print(totalWeight, 2);

  Serial.println(" g");

  //////////////////////////////////////////////////////
  // NO TRASH
  //////////////////////////////////////////////////////

  if (totalWeight <= 10) {

    if (!needTare) {

      needTare = true;
      scale1.tare();
    }

    lcd.setCursor(0, 0);
    lcd.print("No Trash       ");

    lcd.setCursor(0, 1);
    lcd.print("               ");

    hasBeeped = false;

    delay(500);

    return;
  }

  needTare = false;

  //////////////////////////////////////////////////////
  // TRASH DETECTING
  //////////////////////////////////////////////////////

  if (!hasBeeped) {

    hasBeeped = true;

    float capturedWeight = totalWeight;

    Serial.print("Have trash! Captured Weight: ");
    Serial.println(capturedWeight);

    int sensorState = HIGH;

    unsigned long startTime = millis();

    while (millis() - startTime < 4000) {

      if (digitalRead(Inductive_Pin) == LOW) {

        sensorState = LOW;
        break;
      }

      delay(10);
    }

    digitalWrite(LED_RED, HIGH);

    //////////////////////////////////////////////////////
    // METAL
    //////////////////////////////////////////////////////

    if (sensorState == LOW) {

      

      Serial.println("Metal detected");

      //////////////////////////////////////////////////////
      // MQTT SEND
      //////////////////////////////////////////////////////

      count_metal++;

      client.publish("trash/bin1/type", "Metal");

      client.publish("trash/bin1/weight",
                     String(capturedWeight).c_str());

      client.publish("trash/bin1/distance",
                     String(d1).c_str());

      client.publish("trash/bin1/count_metal",
                     String(count_metal).c_str());

      client.publish("trash/bin1/count_plastic",
                     String(count_plastic).c_str());

      client.publish("trash/bin1/count_glass",
                     String(count_glass).c_str());

      Serial.println("MQTT -> Metal data sent");

      lcd.setCursor(0, 0);
      lcd.print("Metal detected ");

      lcd.setCursor(0, 1);
      lcd.print("W: ");
      lcd.print(capturedWeight, 1);
      lcd.print("g        ");

      if (d1 < 10 && d1 > 0) {

        lcd.setCursor(0, 1);
        lcd.print("BIN FULL!!!   ");

        alarmBeep();

        delay(1000);

        hasBeeped = false;
        needTare = false;

        return;

      } else {

        Serial.print("Servo Work");

        shortBeep();

        Servo1.write(160);

        unsigned long t1 = millis();

        while (digitalRead(HALL1_pin) != LOW) {

          if (millis() - t1 > 3000) {

            Serial.println("Hall1 timeout");
            break;
          }

          delay(10);
        }

        Servo1.write(90);

        delay(1000);

        Servo2.write(60);

        digitalWrite(LED_RED, LOW);

        delay(2000);

        scale1.tare();

        hasBeeped = false;
        needTare = true;

        Servo2.write(32);

        delay(1000);

        scale1.tare();

        Servo1.write(0);

        unsigned long t2 = millis();

        while (digitalRead(HALL2_pin) != LOW) {

          if (millis() - t2 > 3000) {

            Serial.println("Hall2 timeout");
            break;
          }

          delay(10);
        }

        Servo1.write(90);
      }

    } else {

      Serial.println("No metal");

      //////////////////////////////////////////////////////
      // PLASTIC
      //////////////////////////////////////////////////////

      if (capturedWeight < 40) {


        Serial.println("Plastic detected");

        //////////////////////////////////////////////////////
        // MQTT SEND
        //////////////////////////////////////////////////////

        count_plastic++;

        client.publish("trash/bin1/type", "Plastic");

        client.publish("trash/bin1/weight",
                       String(capturedWeight).c_str());



        client.publish("trash/bin1/count_metal",
                       String(count_metal).c_str());

        client.publish("trash/bin1/count_plastic",
                       String(count_plastic).c_str());

        client.publish("trash/bin1/count_glass",
                       String(count_glass).c_str());

        Serial.println("MQTT -> Plastic data sent");

        lcd.setCursor(0, 0);
        lcd.print("Plastic detect ");

        lcd.setCursor(0, 1);
        lcd.print("W: ");
        lcd.print(capturedWeight, 1);
        lcd.print("g        ");

        if (d2 < 10 && d2 > 0) {

          lcd.setCursor(0, 1);
          lcd.print("BIN FULL!!!   ");

          alarmBeep();

          delay(1000);

          hasBeeped = false;
          needTare = false;

          return;

        } else {

          shortBeep();

          Servo2.write(60);

          digitalWrite(LED_RED, LOW);

          delay(2000);

          scale1.tare();

          hasBeeped = false;
          needTare = true;

          Servo2.write(32);

          delay(1000);

          scale1.tare();

          Serial.println("plastic already dropped by tan");
        }

      //////////////////////////////////////////////////////
      // GLASS
      //////////////////////////////////////////////////////

      } else {


        Serial.println("Glass detected");

        //////////////////////////////////////////////////////
        // MQTT SEND
        //////////////////////////////////////////////////////

        count_glass++;

        client.publish("trash/bin1/type", "Glass");

        client.publish("trash/bin1/weight",
                       String(capturedWeight).c_str());

        client.publish("trash/bin1/distance",
                       String(d3).c_str());

        client.publish("trash/bin1/count_metal",
                       String(count_metal).c_str());

        client.publish("trash/bin1/count_plastic",
                       String(count_plastic).c_str());

        client.publish("trash/bin1/count_glass",
                       String(count_glass).c_str());

        Serial.println("MQTT -> Glass data sent");

        lcd.setCursor(0, 0);
        lcd.print("Glass detected ");

        lcd.setCursor(0, 1);
        lcd.print("W: ");
        lcd.print(capturedWeight, 1);
        lcd.print("g        ");

        if (d3 < 10 && d3 > 0) {

          lcd.setCursor(0, 1);
          lcd.print("BIN FULL!!!   ");

          alarmBeep();

          delay(1000);

          hasBeeped = false;
          needTare = false;

          return;

        } else {

          shortBeep();

          Serial.print("Servo Work by tan");

          Servo1.write(20);

          unsigned long t3 = millis();

          while (digitalRead(HALL3_pin) != LOW) {

            if (millis() - t3 > 3000) {

              Serial.println("Hall3 timeout");
              break;
            }

            delay(10);
          }

          Servo1.write(90);

          delay(1000);

          Servo2.write(60);

          digitalWrite(LED_RED, LOW);

          delay(2000);

          scale1.tare();

          hasBeeped = false;
          needTare = true;

          Servo2.write(32);

          delay(1000);

          scale1.tare();

          Servo1.write(160);

          unsigned long t2 = millis();

          while (digitalRead(HALL2_pin) != LOW) {

            if (millis() - t2 > 3000) {

              Serial.println("Hall2 timeout");
              break;
            }

            delay(10);
          }

          Servo1.write(90);
        }
      }
    }
  }
client.publish("trash/bin1/distance1",String(60-d1).c_str());
client.publish("trash/bin1/distance2",String(60-d2).c_str());
client.publish("trash/bin1/distance3",String(60-d3).c_str());
  delay(200);
}