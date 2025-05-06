#include <Arduino_APDS9960.h>
#include <Arduino_BMI270_BMM150.h>
#include <ArduinoBLE.h>

#define VALUE_SIZE 50

// Define BLE Service and Characteristics for Proximity, Gesture, Gyroscope, and Accelerometer
BLEService proximityService("00000000-5EC4-4083-81CD-A10B8D5CF6EC");  // Service UUID
BLECharacteristic proximityCharacteristic("00000001-5EC4-4083-81CD-A10B8D5CF6EC", BLERead | BLENotify, VALUE_SIZE); // Proximity
BLECharacteristic gestureCharacteristic("00000002-5EC4-4083-81CD-A10B8D5CF6EC", BLERead | BLENotify, VALUE_SIZE);   // Gesture
BLECharacteristic gyroCharacteristic("00000003-5EC4-4083-81CD-A10B8D5CF6EC", BLERead | BLENotify, VALUE_SIZE);  // Gyroscope
BLECharacteristic accelCharacteristic("00000004-5EC4-4083-81CD-A10B8D5CF6EC", BLERead | BLENotify, VALUE_SIZE);  // Accelerometer

// Threshold values
int proximity = 230;
int oldProximity = 0;
bool tooClose = false;
long previousMillis = 0;
int currentGesture = -1;  // -1 means no gesture detected
float tiltingThreshold = 0;


void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Setup built-in LED and RGB LEDs
  pinMode(LED_BUILTIN, OUTPUT);  // Built-in LED (for braking)
  pinMode(LEDR, OUTPUT); // Red LED
  pinMode(LEDG, OUTPUT); // Green LED
  pinMode(LEDB, OUTPUT); // Blue LED

  while (!Serial);

  // Initialize APDS-9960 sensor
  if (!APDS.begin()) {
    Serial.println("Error initializing APDS-9960 sensor!");
    while (1);
  }

  // Disable LEDs at the start
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  // Initialize BMI270 IMU sensor
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Serial.print("Gyroscope sample rate = ");
  // Serial.print(IMU.gyroscopeSampleRate());
  // Serial.println(" Hz");

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }

  Serial.print("BLE Address: ");
  Serial.println(BLE.address());  // Prints the BLE address of the Arduino

  // Set up BLE service
  BLE.setLocalName("ProximitySensor");
  BLE.setDeviceName("ProximitySensor");

  // Add characteristics to the service
  proximityService.addCharacteristic(proximityCharacteristic);
  proximityService.addCharacteristic(gestureCharacteristic);
  proximityService.addCharacteristic(gyroCharacteristic);
  proximityService.addCharacteristic(accelCharacteristic);  

  // Add service to BLE
  BLE.addService(proximityService);

  // Start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  // Wait for a Bluetooth central (phone, computer, etc.)
  BLEDevice central = BLE.central();

  int gesture;

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, HIGH); // LED indicates connection

    // While the central is connected
    while (central.connected()) {
      long currentMillis = millis();
      if (currentMillis - previousMillis >= 200) {
        previousMillis = currentMillis;
        updateProximity();
        updateGesture();
        updateGyroscope();
        updateAccelerometer();  // Call the accelerometer update function
      }
    }

    // Turn off the LED after disconnect
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

// Function to update proximity data
void updateProximity() {
  if (tooClose == false) {  
    if (APDS.proximityAvailable()) {
      proximity = APDS.readProximity();
    }

    // If proximity is greater than or equal to 50, keep green LED on
    if (proximity > 50) {
      digitalWrite(LEDG, LOW);  // Turn on green LED
      digitalWrite(LEDR, HIGH); // Turn off red LED
    }
    // If proximity is less than 50, turn on red LED
    else {
      digitalWrite(LEDG, HIGH);  // Turn off green LED
      digitalWrite(LEDR, LOW);   // Turn on red LED
      tooClose = true;
    }

    // If proximity changes, send new value over BLE
    if (tooClose == false) {  
      if (proximity != oldProximity) {
        char buffer[VALUE_SIZE];
        int ret = snprintf(buffer, sizeof buffer, "%d", proximity);
        if (ret >= 0) {
          proximityCharacteristic.writeValue(buffer);  // Send proximity data
          oldProximity = proximity;
        }
      }
    }
  }
}

// Function to update gesture data
void updateGesture() {
  if (APDS.gestureAvailable()) {
    int gesture = APDS.readGesture();
    String gestureStr = "";

    if (gesture != currentGesture) {
      // Update gesture if it's different from the current one
      currentGesture = gesture; // Store the new gesture
      switch (gesture) {
        case GESTURE_UP:
          gestureStr = "Detected UP gesture: Not used";
          break;
        case GESTURE_DOWN:
          gestureStr = "Detected DOWN gesture: Brake and No Steer";
          digitalWrite(LED_BUILTIN, HIGH);  // Indicate braking with built-in LED
          delay(1000);
          digitalWrite(LED_BUILTIN, LOW);
          break;
        case GESTURE_LEFT:
          gestureStr = "Detected LEFT gesture: Brake and Steer Left";
          digitalWrite(LEDB, LOW);  // Turn on blue LED for left steering
          delay(1000);
          digitalWrite(LEDB, HIGH); // Turn off blue LED
          break;
        case GESTURE_RIGHT:
          gestureStr = "Detected RIGHT gesture: Brake and Steer Right";
          digitalWrite(LEDG, LOW);  // Turn on green LED for right steering
          delay(1000);
          digitalWrite(LEDG, HIGH); // Turn off green LED
          break;
        default:
          gestureStr = "NONE";
          break;
      }

      // Send gesture data over BLE
      char gestureBuffer[VALUE_SIZE];
      gestureStr.toCharArray(gestureBuffer, sizeof gestureBuffer);
      gestureCharacteristic.writeValue(gestureBuffer);
    }
  }
}


// Function to update gyroscope data
void updateGyroscope() {
  float x, y, z;
  String message = "";

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    if ((tiltingThreshold == 10 && x >= 200) || (tiltingThreshold == 15 && x <= -200))
    {
      message = "Car stabilized";
        
      // Send message data over BLE
      char messageBuffer[VALUE_SIZE];
      message.toCharArray(messageBuffer, sizeof messageBuffer);
      gyroCharacteristic.writeValue(messageBuffer);
    }

    // Serial.print("Gyroscope X: ");
    // Serial.print(x);
    // Serial.print("\tY: ");
    // Serial.print(y);
    // Serial.print("\tZ: ");
    // Serial.println(z);

    // Send gyroscope data over BLE
    // char gyroBuffer[VALUE_SIZE];
    // int ret = snprintf(gyroBuffer, sizeof gyroBuffer, "X: %.2f, Y: %.2f, Z: %.2f", x, y, z);
    // if (ret >= 0) {
    //   gyroCharacteristic.writeValue(gyroBuffer);  // Send gyroscope data
    // }
  }
}

// Function to update accelerometer data
void updateAccelerometer() {
  float ax, ay, az; 
  String message = "", secondMessage = "";

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    if (ax > 1 && currentGesture == GESTURE_DOWN) { // if the car is fast and you are braking
      tiltingThreshold = 5;
      message = "Braking too hard at fast speed!";
      if (tiltingThreshold == 5)
        secondMessage = "Tilt car down!"; // y = -200

      char messageBuffer[VALUE_SIZE];
      message.toCharArray(messageBuffer, sizeof messageBuffer);
      accelCharacteristic.writeValue(messageBuffer);

      // Send secondMessage data over BLE
      char secondMessageBuffer[VALUE_SIZE];
      secondMessage.toCharArray(secondMessageBuffer, sizeof secondMessageBuffer);
      accelCharacteristic.writeValue(secondMessageBuffer);
    }
    else if (ax > 1 && currentGesture == GESTURE_LEFT) { // if the car is fast and you are braking and steering left
      tiltingThreshold = 10;
      message = "Car is tilting too left!";

      if (tiltingThreshold == 10)
        secondMessage = "Tilt car right!"; // x >= 200
            // Send message data over BLE
      char messageBuffer[VALUE_SIZE];
      message.toCharArray(messageBuffer, sizeof messageBuffer);
      accelCharacteristic.writeValue(messageBuffer);

      // Send secondMessage data over BLE
      char secondMessageBuffer[VALUE_SIZE];
      secondMessage.toCharArray(secondMessageBuffer, sizeof secondMessageBuffer);
      accelCharacteristic.writeValue(secondMessageBuffer);
    }
    else if (ax > 1 && currentGesture == GESTURE_RIGHT) { // if the car is fast and you are braking and steering right
      tiltingThreshold = 15;
      message = "Car is tilting too right!";
      if (tiltingThreshold == 15)
        secondMessage = "Tilt car left!"; // x <= -200


    // Send message data over BLE
    char messageBuffer[VALUE_SIZE];
    message.toCharArray(messageBuffer, sizeof messageBuffer);
    accelCharacteristic.writeValue(messageBuffer);

    // Send secondMessage data over BLE
    char secondMessageBuffer[VALUE_SIZE];
    secondMessage.toCharArray(secondMessageBuffer, sizeof secondMessageBuffer);
    accelCharacteristic.writeValue(secondMessageBuffer);

    }

    // Print the accelerometer values to the Serial Monitor (optional)
    // Serial.print("Accelerometer X: ");
    //Serial.print(ax);
    //Serial.print("\tY: ");
    //Serial.print(ay);
    //Serial.print("\tZ: ");
    //Serial.println(az);

    // // Send accelerometer data over BLE
    // char accelBuffer[VALUE_SIZE];
    // int ret = snprintf(accelBuffer, sizeof accelBuffer, "X: %.2f, Y: %.2f, Z: %.2f", ax, ay, az);
    // if (ret >= 0) {
    //   accelCharacteristic.writeValue(accelBuffer);  // Send accelerometer data
    // }
  }
}

