/*
 * 
 *  Run at 20MHz for max logging speed
  SD card datalogger
  SD Card must be formatted FAT32; exFAT will not work at the moment
  created  23 Feb 2022
  modified 16 Apr 2023
  by Euan Mutch (TechAUmNu)

*/
#define MAX_LINES_PER_FILE 50000 // Stops VESC Tool crashing when opening very long logs

#include "Arduino.h"
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include "VescUart.h"
#include <Comparator.h>
#include <Logic.h>
#include <Event.h>
#include <Wire.h>

#define wdt_reset() __asm__ __volatile__ ("wdr"::)
#define ATTINY3216_HANDSHAKE_REPLY 0x71


const int LED = 4;
const int chipSelect = 13;
const int cardDetect = 16;
byte boots;

VescUart UART;
File logFile;

void resetOvercurrent(int numBytes);
void processHandshake();

void setup() {
  _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_4KCLK_gc); //enable the WDT, 4s https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/Ref_Reset.md
  wdt_reset(); 
  
  pinMode(LED, OUTPUT);
  pinMode(cardDetect, INPUT);
  digitalWrite(LED, 0);  

  // Check the watchdog actually works
  //uint8_t resetflags = GPIOR0;
  //if (resetflags == RSTCTRL_WDRF_bm){
  //  while(true);
  //}
  
  // Event channel for reseting faults
  Event1.set_generator(0xFF); // Dodgy but works; if set to disabled it constantly triggers
  Event1.set_user(user::ccl1_event_a);
  Event1.start();

  // Initialize logic block 1
  Logic1.enable = true;                // Enable logic block 1
  Logic1.input0 = logic::in::event_a;         // Connect input 0 to ccl1_event_a (PB2 through Event1)
  Logic1.input1 = logic::in::masked;
  Logic1.input2 = logic::in::masked;
  Logic1.output = logic::out::disable;
  Logic1.truth = 0xFE;                  // Set truth table
  Logic1.init();

  // Configure relevant comparator parameters
  Comparator0.input_p = comparator::in_p::in2;      // Use positive input 2 (PB1)
  Comparator1.input_p = comparator::in_p::in3;      // Use positive input 3 (PB4)
  Comparator2.input_p = comparator::in_p::in1;      // Use positive input 1 (PB0)
  
  // Initialize comparators
  Comparator0.init();
  Comparator1.init();
  Comparator2.init();

  // Configure logic block
  Logic0.enable = true;
  Logic0.input0 = logic::in::ac0;
  Logic0.input1 = logic::in::ac1;
  Logic0.input2 = logic::in::ac2;
  Logic0.output = logic::out::enable;
  Logic0.filter = logic::filter::filter;
  Logic0.sequencer = logic::sequencer::rs_latch; // Latch output
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
  Wire.begin(0x69);                 // join i2c bus with address 0x69
  Wire.onReceive(resetOvercurrent);
  Wire.onRequest(processHandshake);


  //Serial.swap(1); // Not required on V4.1 logic board
  Serial.begin(1000000);
  while (!Serial) {
    wdt_reset();
    delay(1);      
  }
  UART.setSerialPort(&Serial);
  SPI.swap(1);

  // create new file each boot
  boots =  EEPROM.read(0);
  EEPROM.update(0, ++boots);
}

void resetOvercurrent(int numBytes) {
  switch (Wire.read()) { // Check for correct reset code
    case 0x53:
      Event1.soft_event();  
      break;  
  } 
}

void processHandshake() {   
 Wire.write(ATTINY3216_HANDSHAKE_REPLY);  // Handshake to enable gate driver
}


byte subFile = 0; // creates a new file each time sd card is replugged while in the same boot

void loop() {
  while (digitalRead(cardDetect)) {// Wait for card
    wdt_reset();
    digitalWrite(LED, 1);
    delay(1000);
    digitalWrite(LED, 0);
    delay(1000);
  }  
  while (!SD.begin(chipSelect)) { // Wait for card to connect
    wdt_reset();
    delay(1); 
  }    
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
    wdt_reset();
    digitalWrite(LED,  1);
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
  logFile.println("ms_today;input_voltage;temp_mos_max;temp_mos_1;temp_mos_2;temp_mos_3;temp_motor;current_motor;current_in;d_axis_current;q_axis_current;erpm;duty_cycle;amp_hours_used;amp_hours_charged;watt_hours_used;watt_hours_charged;tachometer;tachometer_abs;encoder_position;fault_code;vesc_id;d_axis_voltage;q_axis_voltage;ms_today_setup;amp_hours_setup;amp_hours_charged_setup;watt_hours_setup;watt_hours_charged_setup;battery_level;battery_wh_tot;current_in_setup;current_motor_setup;speed_meters_per_sec;tacho_meters;tacho_abs_meters;num_vescs;ms_today_imu;roll;pitch;yaw;accX;accY;accZ;gyroX;gyroY;gyroZ;gnss_posTime;gnss_lat;gnss_lon;gnss_alt;gnss_gVel;gnss_vVel;gnss_hAcc;gnss_vAcc;");

  long lineCount = 0;
  bool ledState = false;

  // Logging loop
  while (true) {
    wdt_reset();
    if (lineCount % 10 == 1) { // Flash LED to show logging
      digitalWrite(LED, ledState = !ledState);
    }
    if ( UART.getVescSetupValues() ) {
      UART.getVescValues();
      UART.getIMUValues();
      logFile.print(UART.dataSetupValues.setupValTime); // Timestamp
      logFile.print(";");
      logFile.print(UART.dataValues.input_voltage);
      logFile.print(";");
      logFile.print(UART.dataValues.temp_mos_max);
      logFile.print(";");
      logFile.print(UART.dataValues.temp_mos_1);
      logFile.print(";");
      logFile.print(UART.dataValues.temp_mos_2);
      logFile.print(";");
      logFile.print(UART.dataValues.temp_mos_3);
      logFile.print(";");
      logFile.print(UART.dataValues.temp_motor);
      logFile.print(";");
      logFile.print(UART.dataValues.current_motor);
      logFile.print(";");
      logFile.print(UART.dataValues.current_in);
      logFile.print(";");     
      logFile.print(UART.dataValues.d_axis_current);
      logFile.print(";");
      logFile.print(UART.dataValues.q_axis_current);
      logFile.print(";");
      logFile.print(UART.dataValues.erpm);
      logFile.print(";");
      logFile.print(UART.dataValues.duty_cycle);
      logFile.print(";");
      logFile.print(UART.dataValues.amp_hours_used);
      logFile.print(";");
      logFile.print(UART.dataValues.amp_hours_charged);
      logFile.print(";");
      logFile.print(UART.dataValues.watt_hours_used);
      logFile.print(";");
      logFile.print(UART.dataValues.watt_hours_charged);
      logFile.print(";");
      logFile.print(UART.dataValues.tachometer);
      logFile.print(";");
      logFile.print(UART.dataValues.tachometer_abs);
      logFile.print(";");
      logFile.print(UART.dataValues.encoder_position);
      logFile.print(";");
      logFile.print(UART.dataValues.fault_code);
      logFile.print(";");
      logFile.print(UART.dataValues.vesc_id);
      logFile.print(";");
      logFile.print(UART.dataValues.d_axis_voltage);
      logFile.print(";");
      logFile.print(UART.dataValues.q_axis_voltage);
      logFile.print(";");
      
      // Setup data
      logFile.print(UART.dataSetupValues.setupValTime);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.ah_total);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.ah_charge_total);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.wh_total);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.wh_charge_total);      
      logFile.print(";");
      logFile.print(UART.dataSetupValues.battery_level);      
      logFile.print(";");
      logFile.print(UART.dataSetupValues.wh_battery_left);    
      logFile.print(";");      
      logFile.print(UART.dataSetupValues.current_in);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.current_motor);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.speed);
      logFile.print(";"); 
      logFile.print(UART.dataSetupValues.distance);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.distance_abs);
      logFile.print(";");
      logFile.print(UART.dataSetupValues.num_vescs);
      logFile.print(";");

      logFile.print(UART.dataSetupValues.setupValTime);
      logFile.print(";");
      logFile.print(UART.dataIMU.rpy0);
      logFile.print(";");
      logFile.print(UART.dataIMU.rpy1);
      logFile.print(";");
      logFile.print(UART.dataIMU.rpy2);
      logFile.print(";");

      logFile.print(UART.dataIMU.acc0);
      logFile.print(";");
      logFile.print(UART.dataIMU.acc1);
      logFile.print(";");
      logFile.print(UART.dataIMU.acc2);
      logFile.print(";");

      logFile.print(UART.dataIMU.gyro0);
      logFile.print(";");
      logFile.print(UART.dataIMU.gyro1);
      logFile.print(";");
      logFile.print(UART.dataIMU.gyro2);

      // Could add gps data later
      logFile.println(";-1;0.00000000;0.00000000;0.00000000;0.00000000;0.00000000;0.00000000;0.00000000;");
      
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
