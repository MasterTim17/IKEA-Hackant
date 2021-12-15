// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Arduino.h>
#include "avr_util.h"
#include "custom_defs.h"
#include "hardware_clock.h"
#include "io_pins.h"
#include "lin_processor.h"
#include "system_clock.h"
#include <EEPROM.h>

uint16_t lastPosition = 0;
uint16_t memOne = 0;
uint16_t memTwo = 0;

uint16_t currentTarget = 0;
uint8_t initializedTarget = false;
uint8_t targetThreshold = 0;
uint8_t currentTableMovement = 0;

const int moveUp = A1;
const int moveUpCheck = A0;
const int moveDown = A2;
const int moveDownCheck = A3;

bool upState = false;
bool downState = false;

int pressedButton = 0;
int lastPressedButton = 0;
unsigned long lastPressed = 0;
unsigned long lastReleased = 0;
uint8_t clickCount = 0;
uint8_t autoMove = false;


void printValues() {
  Serial.println("======= VALUES =======");
  Serial.print("Memory 1 is at: ");
  Serial.println(memOne);
  Serial.print("Memory 2 is at: ");
  Serial.println(memTwo);
  Serial.print("Threshold is: ");
  Serial.println(targetThreshold);
  Serial.print("Current Position: ");
  Serial.println(lastPosition);
  Serial.println("======================");
}

void printHelp() {
  Serial.println("======= Serial Commands =======");
  Serial.println("Send 'STOP' to stop");
  Serial.println("Send 'HELP' to show this view");
  Serial.println("Send 'VALUES' to show the current values");
  Serial.println("Send 'T123' to set the threshold to 123 (255 max!)");
  Serial.println("Send 'M1' to move to position stored in memory 1");
  Serial.println("Send 'M2' to move to position stored in memory 2");
  Serial.println("Send 'S1' to store current position in memory 1");
  Serial.println("Send 'S2' to store current position in memory 2");
  Serial.println("Send 'M15000' to move set the mem 1 position to 5000");
  Serial.println("Send 'M25000' to move set the mem 2 position to 5000");
  Serial.println("Send '1580' to move to position 1580.");
  Serial.println("===============================");
}

void storeM1(uint16_t value) {
  if (value > 150 && value < 6400) {
    memOne = value;
    Serial.print("New Memory 1: ");
    Serial.println(value);
    EEPROM.put(1, value);
  } else {
    Serial.println("Not stored. Keep your value between 150 and 6400");
  }
}

void storeM2(uint16_t value) {
  if (value > 150 && value < 6400) {
    memTwo = value;
    Serial.print("New Memory 2: ");
    Serial.println(value);
    EEPROM.put(3, value);
  } else {
    Serial.println("Not stored. Keep your value between 150 and 6400");
  }
}

void storeThreshold(uint8_t value) {
  if (value > 50 && value < 254) {
    targetThreshold = value;
    Serial.print("New Threshold: ");
    Serial.println(value);
    EEPROM.put(0, value);
  } else {
    Serial.println("Not stored. Keep your value between 50 and 254");
  }
}


// direction == 0 => Table stops
// direction == 1 => Table goes upwards
// direction == 2 => Table goes downwards
//
void moveTable(uint8_t direction) {
  if (direction != currentTableMovement) {
    currentTableMovement = direction;
    if (direction == 0) {
      Serial.println("Table stops");
      pinMode(moveUp, INPUT);
      pinMode(moveDown, INPUT);
    } else if (direction == 1) {
      Serial.println("Table goes up");
      pinMode(moveUp, OUTPUT);
      digitalWrite(moveUp, 0);
      pinMode(moveDown, INPUT);
    } else {
      Serial.println("Table goes down");
      pinMode(moveUp, INPUT);
      pinMode(moveDown, OUTPUT);
      digitalWrite(moveDown, 0);
    }
  }
}

// direction == 0 => Table is levelled
// direction == 1 => Target is above table
// direction == 2 => Target is below table
uint8_t desiredTableDirection() {

  int distance = lastPosition - currentTarget;
  uint16_t absDistance = abs(distance);

  if (absDistance > targetThreshold) {
    if (distance <= 0) { // table has to move up
      return 1;
    }
    return 2;
  }
  return 0;

}


void processLINFrame(LinFrame frame) {
  // Get the first byte which is the LIN ID
  uint8_t id = frame.get_byte(0);

  // 0x92 is the ID of the LIN node that sends the table position
  if (id == 0x92) {

    // the table position is a two byte value. LSB is sent first.
    uint8_t varA = frame.get_byte(2); //1st byte of the value (LSB)
    uint8_t varB = frame.get_byte(1); //2nd byte (MSB)
    uint16_t temp = 0;

    temp = varA;
    temp <<= 8;
    temp = temp | varB;

    if (temp != lastPosition) {
      lastPosition = temp;
      String myString = String(temp);
      char buffer[5];
      myString.toCharArray(buffer, 5);
      Serial.print("Current Position: ");
      Serial.println(buffer);

      if (initializedTarget == false) {
        currentTarget = temp;
        initializedTarget = true;
      }
    }

  }
}

void readButtons() {

  if (!digitalRead(moveUpCheck)) {
    pressedButton = moveUpCheck;
    if (!upState){
      Serial.println("Button UP Pressed");
      lastPressed = millis();
      upState = true; 
      lastPressedButton = pressedButton;
    }
  }else{
    if (upState){
      pressedButton = moveUpCheck;
      Serial.println("Button UP Relased");
      lastReleased = millis();
      upState = false;
    }
  }

  if (!digitalRead(moveDownCheck)) {
    pressedButton = moveDownCheck;
    if (!downState){
      Serial.println("Button DN Pressed");
      lastPressed = millis();
      downState = true; 
      lastPressedButton = pressedButton;
    }
  }else{
    if (downState){
      pressedButton = moveDownCheck;
      Serial.println("Button DN Relased");
      lastReleased = millis();
      downState = false;
    }
  }

  if(autoMove){
    // read analog voltage for detecting a manual button press and stop moving table
    if(analogRead(moveUpCheck)<50 || analogRead(moveDownCheck)<50){
      currentTarget = lastPosition;
      autoMove = false;
      Serial.println("manual stop");
    }
  } else {
    // enabled manual move -> keep target to position
    currentTarget = lastPosition;
  }

}

void loopButtons() {

  // detect short press
  if(pressedButton != 0){
    if (lastReleased-lastPressed < 300){
      clickCount++;
      lastPressed = 0;
      pressedButton = 0;
    }
  }

  // if clicks finished, check number of clicks
  if(millis()-lastReleased > 500){
    if(clickCount == 2){
      Serial.println("2 Clicks");
      autoMove = true;
      if (lastPressedButton == moveUpCheck) {
        Serial.println("Move to Mem1");
        currentTarget = memOne;
      } else if (lastPressedButton == moveDownCheck) {
        Serial.println("Move to Mem2");
        currentTarget = memTwo;
      }
    }else if(clickCount == 3){
      Serial.println("3 Clicks");
      if (lastPressedButton == moveUpCheck) {
        Serial.println("Store Mem1");
        storeM1(lastPosition);
      } else if (lastPressedButton == moveDownCheck) {
        Serial.println("Store Mem2");
        storeM2(lastPosition);
      }
    }
    clickCount = 0;
    lastPressedButton = 0;
  }
    
}


void setup() {

  Serial.begin(115200);
  while (!Serial) {;};

  Serial.println("IKEA Hackant v2 by MasterTim17");
  Serial.println("Type 'HELP' to display all commands.");

  pinMode(moveUp, INPUT); // will switch to output during runtime
  pinMode(moveDown, INPUT); // will switch to output during runtime
  pinMode(moveUpCheck, INPUT);
  pinMode(moveDownCheck, INPUT);

  Serial.print("moveUpButton ");
  Serial.println(moveUp);
  Serial.print("moveDownButton ");
  Serial.println(moveDown);

  // setup everything that the LIN library needs.
  hardware_clock::setup();
  lin_processor::setup();

  // Enable global interrupts.
  sei();




  EEPROM.get(0, targetThreshold);
  if (targetThreshold == 255) {
    storeThreshold(120);
  }

  EEPROM.get(1, memOne);
  if (memOne == 65535) {
    storeM1(3500);
  }


  EEPROM.get(3, memTwo);
  if (memTwo == 65535) {
    storeM2(3500);
  }

  printValues();

}



void loop() {


  // Periodic updates.
  system_clock::loop();


  // Handle recieved LIN frames.
  LinFrame frame;

  // if there is a LIN frame
  if (lin_processor::readNextFrame(&frame)) {
    processLINFrame(frame);
  }

  // direction == 0 => Table is levelled
  // direction == 1 => Target is above table
  // direction == 2 => Target is below table
  uint8_t direction = desiredTableDirection();
  moveTable(direction);


  if (Serial.available() > 0) {

    // read the incoming byte:
    String val = Serial.readString();

    if (val.indexOf("HELP") != -1 || val.indexOf("help") != -1) {
      printHelp();
    } else if (val.indexOf("VALUES") != -1 || val.indexOf("values") != -1) {
      printValues();
    } else if (val.indexOf("STOP") != -1 || val.indexOf("stop") != -1) {

      if (direction == 1)
        currentTarget = lastPosition + (targetThreshold * 2);
      else if (direction == 2)
        currentTarget = lastPosition - (targetThreshold * 2);

      Serial.print("STOP at ");
      Serial.println(currentTarget);


    } else if (val.indexOf('T') != -1 || val.indexOf("t") != -1) {
      uint8_t threshold = (uint8_t)val.substring(1).toInt();
      storeThreshold(threshold);

    } else if (val.indexOf("M1") != -1 || val.indexOf("m1") != -1) {

      if (val.length() == 2) {
        currentTarget = memOne;
      } else {
        storeM1(val.substring(2).toInt());
      }


    } else if (val.indexOf("M2") != -1 || val.indexOf("m2") != -1) {

      if (val.length() == 2) {
        currentTarget = memTwo;
      } else {
        storeM2(val.substring(2).toInt());
      }

    } else if (val.indexOf("S1") != -1 || val.indexOf("s1") != -1) {

      storeM1(lastPosition);

    } else if (val.indexOf("S2") != -1 || val.indexOf("s2") != -1) {

      storeM2(lastPosition);

    } else {
      if (val.toInt() > 150 && val.toInt() < 6400) {
        Serial.print("New Target ");
        Serial.println(val);
        currentTarget = val.toInt();
      } else {
        Serial.println("Not stored. Keep your value between 150 and 6400");
      }
    }
  }

  readButtons();
  loopButtons();

}
