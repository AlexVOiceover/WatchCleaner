#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <./Fonts/Dialog_bold_16.h>

#define SCREEN_WIDTH 128 // OLED display width
#define SCREEN_HEIGHT 64 // OLED display height

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Motor control pins
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;

// PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;

// Motor speed settings
float currentSpeed = 150.0; // Start at minimum operational speed
const float minSpeed = 155.0; 
float speedIncrement = 0.5;
int gradualDelay = 3;

// Button01
const int button01 = 19;

// Button02
const int button02 = 5;

//test

// Selected program by default
int selectedProgram = 1;

// Will record if a program is running
bool programRunning = false;

// Program step structure
struct ProgramStep {
  int speed;      // Duty cycle (150 to 255)
  int duration;   // Duration in seconds
  int direction;  // Motor direction (0 or 1)
};

struct MotorProgram {
  String title;
  ProgramStep* steps;
  int stepCount;
};

// When defining program1Steps and program1
//ProgramStep program1Steps[] = {{190, 5, 0}, {190, 5, 1}, {190, 5, 0},{190, 2, 0}, {190, 2, 1}, {190, 2, 0}};
ProgramStep program1Steps[] = {{190, 2, 1}, {190, 2, 0}};
MotorProgram program1 = {"Pre-Clean", program1Steps,2};

ProgramStep program2Steps[] = {{190, 2, 1}, {190, 2, 0}};
MotorProgram program2 = {"Clean", program2Steps,2};

ProgramStep program3Steps[] = {{190, 2, 1}, {190, 2, 0}};
MotorProgram program3 = {"Rinse 1", program3Steps,2};

ProgramStep program4Steps[] = {{190, 2, 1}, {190, 2, 0}};
MotorProgram program4 = {"Rinse 2", program4Steps,2};

ProgramStep program5Steps[] = {{190, 2, 1}, {190, 2, 0}};
MotorProgram program5 = {"Dry", program5Steps,2};


MotorProgram programs[] = {program1, program2, program3, program4, program5};
// Calculate the number of programs
const int numberOfPrograms = sizeof(programs) / sizeof(MotorProgram);


void setup() {
  // Initialize motor control pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  
  // Set up PWM
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);

   // Initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // 0x3C is the I2C address

 showMenu();

 Serial.begin(115200);

  
  // Initialize the buttons pin as an input
  pinMode(button01, INPUT_PULLUP);
  pinMode(button02, INPUT_PULLUP);


  Serial.begin(115200); // Uncomment for debugging
}

void loop() {
 // Handle program selection
    if (digitalRead(button02) == LOW) {
        delay(50); // Debounce delay
        if (digitalRead(button02) == LOW) {
            selectedProgram = (selectedProgram % numberOfPrograms) + 1; // Cycle through programs
            showMenu(); // Function to update the menu display
            while (digitalRead(button02) == LOW); // Wait for button release
        }
    }

    // Check if the button is pressed
    if (digitalRead(button01) == LOW) {
        // Debouncing delay
        delay(50);
        // Check if the button is still pressed
        if (digitalRead(button01) == LOW) {
            programRunning = !programRunning; // Toggle the state
             Serial.print(programRunning);
            while (digitalRead(button01) == LOW); // Wait for the button to be released
        }
    }

    if (programRunning) {
        executeProgram(programs[selectedProgram - 1]); // Execute the program only if programRunning is true
    }
}

void executeProgram(MotorProgram program) {

  int stepCount = program.stepCount;
  int totalTime = 0;

  // Calculate total program time
   for (int i = 0; i < program.stepCount; i++) {
    totalTime += program.steps[i].duration;
  }

  int currentStepIndex = 0;
  unsigned long lastSecondUpdate = millis();
  
  // Main control loop
  while (currentStepIndex < program.stepCount) {

    ProgramStep currentStep = program.steps[currentStepIndex];
    unsigned long stepEndTime = millis() + (currentStep.duration * 1000);

    // Decelerate to minimum speed before changing speed or direction
    decelerateToMinSpeed();

    // Change motor direction if needed
    changeMotorDirection(currentStep.direction);

    // Gradually ramp speed to the target for the current step
    setMotorSpeed(currentStep.speed, currentStep.direction);

    // Step execution loop
    while (millis() < stepEndTime) {
      // Update every second
      if (millis() - lastSecondUpdate >= 1000) {
        lastSecondUpdate = millis();
        totalTime--;

        // Display countdown timer
        int minutes = totalTime / 60;
        int seconds = totalTime % 60;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(program.title);
        display.setCursor(0, 20);
        display.setTextSize(4);
        if (minutes < 10) display.print('0');
        display.print(minutes);
        display.print(':');
        if (seconds < 10) display.print('0');
        display.print(seconds);
        display.display();
      }

      // Button check to interrupt the program
      if (digitalRead(button01) == LOW) {
          delay(50); // Debounce delay
          if (digitalRead(button01) == LOW) {
              Serial.println("Program interrupted.");
              stopMotor();
              programRunning = false; // Update the state
              showMenu();
              return; // Exit the function
          }
      }

    }

    // Move to the next step
    currentStepIndex++;
  }

  //Show message program done
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.print("Finished");
      display.setCursor(0, 20);
      display.setTextSize(4);
      display.print("00:00");
      display.display();
  // Stop the motor and end the program
  stopMotor();
  programRunning = false;
  delay(5000);
  showMenu();

}

void setMotorSpeed(int speed, int direction) {
    // Set motor direction
    digitalWrite(motor1Pin1, direction == 0 ? LOW : HIGH);
    digitalWrite(motor1Pin2, direction == 0 ? HIGH : LOW);

    // Gradually ramp speed to target
    while (abs(currentSpeed) != abs(speed)) {
        float speedDifference = speed - currentSpeed;

        // Determine the amount to change the speed in this iteration
        float speedChange = (abs(speedDifference) < speedIncrement) ? abs(speedDifference) : speedIncrement;
        currentSpeed += (speedDifference > 0) ? speedChange : -speedChange;

        // Apply the speed change
        currentSpeed = max(minSpeed, min(abs(currentSpeed), 255.0f));
        ledcWrite(pwmChannel, abs((int)currentSpeed));

       delay(gradualDelay); // Small delay for gradual change
    }
}

void stopMotor() {
    // Decelerate motor to minimum speed
    decelerateToMinSpeed();
    // Stop motor
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    ledcWrite(pwmChannel, 0);
    currentSpeed = minSpeed; // Reset current speed
}

void decelerateToMinSpeed() {
    while (abs(currentSpeed) > minSpeed) {
        currentSpeed += (currentSpeed > 0) ? -speedIncrement : speedIncrement;
        ledcWrite(pwmChannel, abs((int)currentSpeed));
        delay(gradualDelay); // Small delay for gradual change
    }
}

void changeMotorDirection(int direction) {
    digitalWrite(motor1Pin1, direction == 0 ? LOW : HIGH);
    digitalWrite(motor1Pin2, direction == 0 ? HIGH : LOW);
}

void showMenu() {
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Watch Cleaner");

    // Display program options with highlighting the selected one
    displayProgramOption(" 1-Pre-clean", 1, 16);
    displayProgramOption(" 2-Clean", 2, 26);
    displayProgramOption(" 3-Rinse 1", 3, 36);
    displayProgramOption(" 4-Rinse 2", 4, 46);
    displayProgramOption(" 5-Dry", 5, 56);
    
    display.display();
}


void displayProgramOption(String programName, int programNumber, int yPosition) {
    display.setCursor(0, yPosition);
    if (programNumber == selectedProgram) {
        display.print("*");
        display.print(programName.substring(1)); // Print the program name without the first space
    } else {
        display.print(programName); // Print the program name as is
    }
}
