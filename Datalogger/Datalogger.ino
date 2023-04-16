/*
  SD card datalogger
  SD Card must be formatted FAT32, exFAT will not work at the moment
  created  23 Feb 2022
  modified 16 Apr 2023
  by Euan Mutch (TechAUmNu)

*/
#define MAX_LINES_PER_FILE 50000 // Stops VESC Tool crashing when opening very long logs

#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include "VescUart.h"
#include <Comparator.h>
#include <Logic.h>
#include <Event.h>
#include <Wire.h>

const int LED = 4;
const int chipSelect = 13;
const int cardDetect = 16;
byte boots;

VescUart UART;
File logFile;


void setup() {
  pinMode(LED, OUTPUT);
  pinMode(cardDetect, INPUT);
  digitalWrite(LED, 1);

  // Event channel for reseting faults
  Event1.set_generator(gens::disable);
  Event1.set_user(user::ccl1_event_a);
  Event1.start();

  // Initialize logic block 1
  Logic1.enable = true;                // Enable logic block 1
  Logic1.input0 = logic_in::event_a;         // Connect input 0 to ccl1_event_a (PB2 through Event1)
  Logic1.input1 = logic_in::masked;
  Logic1.input2 = logic_in::masked;
  Logic1.output = logic_out::disable;
  Logic1.truth = 0xFE;                  // Set truth table
  Logic1.init();

  // Configure relevant comparator parameters
  Comparator0.input_p = comparator_in_p::in2;      // Use positive input 2 (PB1)
  Comparator1.input_p = comparator_in_p::in3;      // Use positive input 3 (PB4)
  Comparator2.input_p = comparator_in_p::in1;      // Use positive input 1 (PB0)

  // Initialize comparators
  Comparator0.init();
  Comparator1.init();
  Comparator2.init();

  // Configure logic block
  Logic0.enable = true;
  Logic0.input0 = logic_in::ac0;
  Logic0.input1 = logic_in::ac1;
  Logic0.input2 = logic_in::ac2;
  Logic0.output = logic_out::enable;
  Logic0.sequencer = logic_sequencer::rs_latch; // Latch output
  Logic0.truth = 0xFE;                    // Set truth table (3 input OR)
  Logic0.init();

  // Start comparators
  Comparator0.start();
  Comparator1.start();
  Comparator2.start();

  // Start the AVR logic hardware
  Logic::start();

  // I2C for resetting overcurrent protection
  Wire.swap(1);
  Wire.begin(0x69);                 // join i2c bus with address 0x69 NICE
  Wire.onReceive(resetOvercurrent);


  //Serial.swap(1); // Not required on V4.1 logic board
  Serial.begin(1000000);
  while (!Serial) {}
  UART.setSerialPort(&Serial);
  SPI.swap(1);

  // create new file each boot
  boots =  EEPROM.read(0);
  EEPROM.update(0, ++boots);
}

void resetOvercurrent(int16_t numBytes) {
  if (Wire.read() == 0x53) { // Check for correct reset code
    Event1.soft_event();
  }
}


byte subFile = 0; // creates a new file each time sd card is replugged while in the same boot

void loop() {
  while (digitalRead(cardDetect)) {}  // Wait for card
  while (!SD.begin(chipSelect)) {}    // Wait for card to connect
  char temp[10];
  // Filename must conform to short DOS 8.3
  char fileName[20];
  strcpy(fileName, itoa(boots, temp, 10));
  strcat(fileName, "-");
  strcat(fileName, itoa(subFile, temp, 10));
  strcat(fileName, ".csv");
  logFile = SD.open(fileName, FILE_WRITE);
  subFile++;


  // Read firmware info and put at start of log file
  while (!UART.getVescFirmwareInfo()) {
    digitalWrite(LED, 1);
    delay(500);
    digitalWrite(LED, 0);
    delay(500);
  }
  // Removed as it crashes vesc tool when opening log
  //logFile.print("FW:");
  //logFile.print(UART.firmware.firmwareVersionMajor);
  //logFile.print(".");
  //logFile.print(UART.firmware.firmwareVersionMinor);
  //logFile.print(" B:");
  //logFile.println(UART.firmware.firmwareVersionBeta);
  //logFile.print("HW:");
  //logFile.println(UART.firmware.hardwareName);
  logFile.println("ms_today,input_voltage,temp_mos_max,temp_mos_1,temp_mos_2,temp_mos_3,temp_motor,current_motor,current_in,d_axis_current,q_axis_current,erpm,duty_cycle,amp_hours_used,amp_hours_charged,watt_hours_used,watt_hours_charged,tachometer,tachometer_abs,encoder_position,fault_code,vesc_id,d_axis_voltage,q_axis_voltage");

  long lineCount = 0;
  bool ledState = false;

  // Logging loop
  while (true) {
    if (lineCount % 10 == 1) { // Flash LED to show logging
      digitalWrite(LED, ledState = !ledState);
    }
    if ( UART.getVescValues() ) {
      logFile.print(millis()); // Timestamp
      logFile.print(",");
      logFile.print(UART.data.input_voltage);
      logFile.print(",");
      logFile.print(UART.data.temp_mos_max);
      logFile.print(",");
      logFile.print(UART.data.temp_mos_1);
      logFile.print(",");
      logFile.print(UART.data.temp_mos_2);
      logFile.print(",");
      logFile.print(UART.data.temp_mos_3);
      logFile.print(",");
      logFile.print(UART.data.temp_motor);
      logFile.print(",");
      logFile.print(UART.data.current_motor);
      logFile.print(",");
      logFile.print(UART.data.current_in);
      logFile.print(",");
      logFile.print(UART.data.d_axis_current);
      logFile.print(",");
      logFile.print(UART.data.q_axis_current);
      logFile.print(",");
      logFile.print(UART.data.erpm);
      logFile.print(",");
      logFile.print(UART.data.duty_cycle);
      logFile.print(",");
      logFile.print(UART.data.amp_hours_used);
      logFile.print(",");
      logFile.print(UART.data.amp_hours_charged);
      logFile.print(",,,,,");
      logFile.print(UART.data.encoder_position);
      logFile.print(",");
      logFile.print(UART.data.fault_code);
      logFile.print(",");
      logFile.print(UART.data.vesc_id);
      logFile.print(",");
      logFile.print(UART.data.d_axis_voltage);
      logFile.print(",");
      logFile.println(UART.data.q_axis_voltage);
      logFile.availableForWrite();
      lineCount++;
    } else {
      digitalWrite(LED, LOW);
    }
    if (lineCount > MAX_LINES_PER_FILE) {
      logFile.close(); // Close the file in the file system
      break;
    }
    if (digitalRead(cardDetect)) {
      logFile.close(); // Close the file in the file system
      break;
    }
  }
}
