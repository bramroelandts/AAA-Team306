/* Note to KU Leuven:
 *   Team 306 is using a HM-10 Bluetooth module, which supports
 *   Bluetooth 4.0 and can be connected with our iPhone app.
 */


// MARK: - Library imports
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DFRobot_HX711.h>
#include <Servo.h>

// Initialize i2c LCD dispslay, at address 0x27
LiquidCrystal_I2C LcdModule(0x27,16,2);
// Initialize the scale on D7 and D8
DFRobot_HX711 ScaleModule(7, 8);
// Initialize the buzzer
const int buzzerPin = 3;

// Initialize servo motors
Servo platformServo;
Servo screwServo;
Servo reservoirServo;


// Define logging settings for various components
// Allows for quick and to-the-point debugging
boolean logBluetoothComm = false;
boolean logLcdDisplay = false;
boolean logServoMovement = false;
boolean logLedAnimation = false;
boolean logBuzzerAnimation = false;
boolean logCrashDetection = false;
boolean logScale = false;
enum SoftwareComponent { bluetooth, lcd, servo, led, buzzer, crash, scale };
enum Resource { erwten, linzen, mais };
enum SpinDirection { forward, backward };


// --------------------------- MARK: - Core layer ---------------------------

// MARK: - Setup
void setup() {

  // Initialize the hardware serial connection with Bluetooth module
  Serial.begin(9600);

  // Initialize the LCD module
  LcdModule.init();
  LcdModule.init();
  LcdModule.backlight();
  
  // Setup the buzzer
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH); // HIGH is off on this module
  
  // Setup the servo pinout
  platformServo.attach(9);
  screwServo.attach(11);
  reservoirServo.attach(10);

  // Get the machine ready for use
  resetMachineState();
}

void resetMachineState() {

  // Set a welcome message
  displayOnLCD("AAA Team 306", "Klaar voor order");
  
  // Write an initial position to the servo's
  platformServo.write(5);
  screwServo.write(100);
  reservoirServo.write(0);
}


// MARK: - Main loop
void loop() {

  // Continuously monitor Bluetooth serial
  String output = monitorBluetooth();

  if (output.length() > 0) {
    log(bluetooth, output);
    handleIncomingOrder(output);
  }
}


// --------------------------- MARK: - Communication layer ---------------------------

String monitorBluetooth() {
  String receivedText = "";
  while (Serial.available()) {
    char c = Serial.read();
    receivedText += c;
    delay(10);
  }
  return receivedText;
}

void notifyApp(float currentWeight) {
  // Bluetooth module is connected to HW serial,
  // to avoid overhead, since we're using the regular Nano
  Serial.print(currentWesight);
}

float retrieveCurrentWeight() {
  return ScaleModule.readWeight();
}


// --------------------------- MARK: - Logic layer ---------------------------

void handleIncomingOrder(String command) {

  // Provide visual feedback
  displayOnLCD("Order ontvangen", "Verwerken...");
  
  // Convert command to raw bytes
  byte commandBuffer[command.length() + 1]; // alloc buffer
  command.getBytes(commandBuffer, command.length() + 1); // parse string into buffer
  
  // Check for length
  if (command.length() == 3) {
    // Check the instruction byte (0x0A for order)
    if (commandBuffer[0] == 0x0A) {

      // Define order details
      Resource chosenResource;
      int desiredWeight = (int)commandBuffer[2];
      
      // Determine the resource type
      switch (commandBuffer[1]) {
        case 0x00: chosenResource = erwten; break;
        case 0x01: chosenResource = linzen; break;
        case 0x02: chosenResource = mais; break;
      }


      // MARK: - Start executing the order
    
      // Position the feed screw according to the requested resource
      positionFeedScrew(chosenResource);
    
      // Start the feeding and keep track of the weight
      startFeeding(float(desiredWeight));
      
      // Remove residue
      unloadResidue();
    
      // Empty the reservoir
      emptyReservoir();
      
    } else {
      warn(bluetooth, "Received an unknown instruction byte.");
    }
  } else {
      warn(bluetooth, "Received a message of length not equal to 3.");
  }
}



// --------------------------- MARK: - Execution layer ---------------------------

void displayOnLCD(String firstLine, String secondLine) {

  // Check for display line overflow
  if (firstLine.length() > 16 or secondLine.length() > 16) {
    warn(lcd, "Content exceeds LCD real estate.");
  }

  // Print out over i2c
  LcdModule.clear();
  LcdModule.setCursor(0,0);
  LcdModule.print(firstLine);
  LcdModule.setCursor(0,1);
  LcdModule.print(secondLine);
}

void positionFeedScrew(Resource resource) {

  // Orient the base platform
  switch (resource) {
    case erwten: orientPlatform(180); break;
    case linzen:  orientPlatform(145); break;
    case mais: orientPlatform(110); break;
  }
  delayWhileAlarming(2500);

  // Lower the feed screw into the resource
  orientScrew(true);
  delayWhileAlarming(1500);
}

void startFeeding(float desiredWeight) {

  int measureInterval = 1000; // Check current weight every 1000ms
  float currentWeight = 0;

  // Allow for a tolerance, should be tested
  while (currentWeight < (desiredWeight - 0.9)) {
    
    // Fetch the current weight
    currentWeight = retrieveCurrentWeight();
    float weightDelta = desiredWeight - currentWeight;

    // Adjust feed speed depending on amount left
    if (weightDelta > 15) { // Full speed
      setScrewSpeed(10, forward);
    } else if (weightDelta < 3) { // Low speed, 3 g left
      setScrewSpeed(3, forward); // Arbitrary, needs testing
      measureInterval = 250; // Speed up measurements
    } else { // Moderate speed, 15 g left
      setScrewSpeed(6, forward);
      measureInterval = 500;
    }

    // Display on the LCD
    displayOnLCD( \
      ("Huidig: " + String(int(currentWeight)) + " g"), \
      ("Gevraagd: " + String(int(desiredWeight)) + " g"));

    // Notify the app
    notifyApp(currentWeight);

    // Wait for the next measuring slot
    // NOTE: Unsure if delay is allowed. Might disrupt DC.
    //       Can't investigate due to corona.
    delay(measureInterval);
  }
  
  // Enough has been loaded
  setScrewSpeed(0, forward);

  // Do a final check of the loaded amount
  currentWeight = retrieveCurrentWeight();
  float weightDelta = desiredWeight - currentWeight;

  if (weightDelta > 1.0) {
    warn(scale, "Weight delta exceeds 1 gram. Try harder next time.");
  }
  
  // Display status
  displayOnLCD( \
      "Klaar", \
      ("Opgeladen: " + String(int(currentWeight)) + " g"));

  delay(1000);
  orientScrew(false);
}

void unloadResidue() {
  
  // Rotate backwards to remove any residue
  setScrewSpeed(10, backward);
  delay(2500); // Don't know if delay plays along with the DC motor
  setScrewSpeed(0, backward);
}

void emptyReservoir() {

  // Re-orient the platform
  orientPlatform(5);
  delayWhileAlarming(2500);

  // Keep the user updated
  displayOnLCD("Order afladen..","");
  delay(1000);
  
  // Empty the reservoir
  orientReservoir(true);
  delay(2500);

  // Get the reservoir back up
  orientReservoir(false);

  // Keep the user updated
  displayOnLCD("Order compleet!","");
  delay(5000);

  // Reset the whole machine back to initial state
  resetMachineState();
}


// --------------------------- MARK: - Motor movement -----------------------------

void setScrewSpeed(int speed, SpinDirection direction) {
  // To be implemented, based on testing
}

void orientPlatform(int degrees) {
  platformServo.write(degrees);
}

void orientReservoir(bool empty) {
  reservoirServo.write(empty ? 100 : 0);
}

void orientScrew(bool intoResource) {
  screwServo.write(intoResource ? 140 : 90);
}

void delayWhileAlarming(int timeInterval) {

  int passed = 0;

  // Keep going as long as we were asked to
  while (passed < timeInterval) {
    // Sound a 1 kHz tone
    tone(3, 1000);
    delay(300);
    // Turn off the sound
    // Both noTone and digitalWrite are required to reach complete silence
    noTone(3);
    digitalWrite(3, HIGH);
    delay(400);

    // Increment the passed time
    passed += 700;
  }
}


// --------------------------- MARK: - Helper functions ---------------------------

struct LoggingSetting {
  boolean shouldLog;
  String componentName;
};

void log(SoftwareComponent component, String output) {

  // Retrieve logging turned on/off and component name
  struct LoggingSetting setting = retrieveLogSettings(component);

  if (setting.shouldLog) {
      Serial.println("[" + setting.componentName + "]: " + output);
  }
}

void warn(SoftwareComponent component, String warning) {
  
  // Retrieve logging turned on/off and component name
  struct LoggingSetting setting = retrieveLogSettings(component);

  // Disregard loggingEnabled here, warnings should always go through
  Serial.println("WARNING: [" + setting.componentName + "]: " + warning);
}

struct LoggingSetting retrieveLogSettings(SoftwareComponent component) {

   LoggingSetting setting;
   
  // Look up whether output should be logged
  switch (component) {
    case bluetooth:   setting.shouldLog = logBluetoothComm;   setting.componentName = "Bluetooth";     break;
    case lcd:         setting.shouldLog = logLcdDisplay;      setting.componentName = "LCD Display";   break;
    case servo:       setting.shouldLog = logServoMovement;   setting.componentName = "Servo";         break;
    case led:         setting.shouldLog = logLedAnimation;    setting.componentName = "LEDs";          break;
    case buzzer:      setting.shouldLog = logBuzzerAnimation; setting.componentName = "Buzzer";        break;
    case crash:       setting.shouldLog = logCrashDetection;  setting.componentName = "CrashDetection";break;
    case scale:       setting.shouldLog = logScale;           setting.componentName = "Scale";         break;
  }
  return setting;
}
