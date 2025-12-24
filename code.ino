#include <Wire.h>
#include <MPU6050.h>
#include <HardwareSerial.h>
#include <Keypad.h>

// MPU & GSM 
MPU6050 mpu;
HardwareSerial sim800(2);   // RX=16, TX=17

// BUZZER
#define BUZZER 25

// KEYPAD
const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};


byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String password = "1234";
String enteredPass = "";
bool systemArmed = false;

// Thresholds
float tiltThreshold = 30.0;
float vibrationThreshold = 15000;
float movementThreshold = 20000;
float crashThreshold = 30000;


void setup() {
  Serial.begin(115200);

  sim800.begin(9600, SERIAL_8N1, 16, 17);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Wire.begin();
  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 not detected!");
    while (1);
  }

  Serial.println("MPU6050 Connected!");
  sendSMS("System Ready. Press * to ARM.");
}

// SEND SMS
void sendSMS(String msg) {
  sim800.println("AT+CMGF=1");
  delay(500);
  sim800.println("AT+CMGS=\"+91XXXXXXXXXX\""); // put your number
  delay(500);
  sim800.print(msg);
  delay(300);
  sim800.write(26);
  delay(3000);
}

// ---------------- LOOP --------------------
void loop() {

  // ----------- KEYPAD HANDLING ------------
  char key = keypad.getKey();

  if (key) {
    Serial.print("Key: ");
    Serial.println(key);

    if (key == '*') {          // ARM
      enteredPass = "";
      Serial.println("Enter Password to ARM:");
    }
    else if (key == '#') {     // CONFIRM
      if (enteredPass == password) {
        systemArmed = !systemArmed;
        digitalWrite(BUZZER, LOW);

        if (systemArmed) {
          Serial.println("SYSTEM ARMED");
          sendSMS("System Armed");
        } else {
          Serial.println("SYSTEM DISARMED");
          sendSMS("System Disarmed");
        }
      } else {
        Serial.println("Wrong Password");
      }
      enteredPass = "";
    }
    else {
      enteredPass += key;  // collect digits
    }
  }

  // IF DISARMED, STOP 
  if (!systemArmed) return;

  // MPU6050 READ
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  // TILT 
  float tilt = atan2(sqrt(axg*axg + ayg*ayg), azg) * 57.3;
  if (tilt > tiltThreshold) {
    Serial.println("TILT DETECTED");
    digitalWrite(BUZZER, HIGH);
    sendSMS("ALERT: Vehicle Tilt Detected");
    delay(5000);
    digitalWrite(BUZZER, LOW);
  }

  // VIBRATION
  if (abs(ax) > vibrationThreshold || abs(ay) > vibrationThreshold) {
    Serial.println("VIBRATION DETECTED");
    sendSMS("ALERT: Vehicle Vibration Detected");
    delay(3000);
  }

  // MOVEMENT
  if (abs(ax) > movementThreshold || abs(ay) > movementThreshold) {
    Serial.println("THEFT MOVEMENT");
    digitalWrite(BUZZER, HIGH);
    sendSMS("ALERT: Vehicle Being Moved");
    delay(5000);
    digitalWrite(BUZZER, LOW);
  }

  // CRASH 
  if (abs(ax) > crashThreshold || abs(ay) > crashThreshold || abs(az) > crashThreshold) {
    Serial.println("CRASH DETECTED");
    digitalWrite(BUZZER, HIGH);
    sendSMS("ALERT: Crash Detected");
    delay(7000);
    digitalWrite(BUZZER, LOW);
  }

  delay(200);
}
