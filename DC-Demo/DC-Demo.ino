/* Note to KU Leuven:
 *   Team 306 is using a TI L293D motor controller chip.
 *   The Arduino Nano delegates technical aspects of controlling the motors to this chip.
 *   We haven't integrated the DC-motor into the main prototype/codebase, because a different
 *   team member (Jasper Pas) carried out DC-motor tests.
 *
 *   The main prototype already demonstrates the Bluetooth module, so this demonstration only
 *   uses a potentiometer as input, controls the motor accordingly and displays speed and direction
 *   on an 8-segment display.
 */

#include <Arduino_Helpers.h> // Include the Arduino Helpers library.
#include <AH/Hardware/LEDs/MAX7219SevenSegmentDisplay.hpp>
#include <AH/Hardware/FilteredAnalog.hpp>
MAX7219SevenSegmentDisplay max7219 = 10;
FilteredAnalog<10, 6, uint32_t, uint32_t> analog = {A3, 512};

const int motorPin1  = 5;  // Pin 14 of L293
const int motorPin2  = 6;  // Pin 10 of L293
// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  max7219.begin();          // Initialize the shift registers
  analog.setupADC();
  int constrained_range = 500;  // The range from the midpoint(512) to the max value you want to constrain your inputrange to
  int min_motorspeed = 55;  //Experimentally determend point where the motor actually starts rotating
  int min_sensorValue = min_motorspeed / 255 * constrained_range;
  int negative_min_sensorValue = 512 - min_sensorValue;
  int negative_max_sensorValue = 512 - constrained_range;
  int positive_min_sensorValue = 512 + min_sensorValue;
  int positive_max_sensorValue = 512 + constrained_range;

  TCCR0B = (TCCR0B & ~0b111) | 0b001;
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:

  if (analog.update()) {
    int sensorValue = analog.getValue();
    if (0 <= sensorValue && sensorValue < negative_min_sensorValue) {
      sensorValue =  512 - constrain(sensorValue, negative_max_sensorValue , negative_min_sensorValue);
      int motorspeed = sensorValue * 255l / 500;
      max7219.display(-motorspeed * 100 / 255);
      analogWrite(motorPin1, motorspeed);
      digitalWrite(motorPin2, LOW);
    }
    else if ( 379 <= sensorValue && sensorValue < positive_min_sensorValue) {
      int motorspeed = 0;
      max7219.display(motorspeed * 100 / 255);
      analogWrite(motorPin1, 0);
      digitalWrite(motorPin2, LOW);

    }
    else {
      sensorValue = constrain(sensorValue, positive_min_sensorValue, positive_max_sensorValue) - 512;
      int motorspeed = sensorValue * 255l / 500;
      max7219.display(motorspeed * 100 / 255);
      analogWrite(motorPin2, motorspeed);
      digitalWrite(motorPin1, LOW);
    }
  }
  delay(1);        // delay in between reads for stability
}
