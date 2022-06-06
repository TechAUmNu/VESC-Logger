/*
  SD card datalogger

  created  23 Feb 2022
  modified 11 Apr 2022
  by Euan Mutch (TechAUmNu)

*/


// TODO:
// Handling for removed/reinserted sd cards - requires switch on card socket to be wired up on pcb!
// Reclaim some space!
// Hardware overcurrent setup
// Hardware overcurrent reset over uart
// Hardware overcurrent logging

#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include "VescUart.h"
#include <Comparator.h>
#include <Logic.h>
#include <Event.h>

const int LED = 4;
const int chipSelect = 13;
const int cardDetect = 16;


VescUart UART;
File logFile, infoFile;

void setup() {

  // Configure event channel for reset
  EVSYS.ASYNCCH1 = EVSYS_ASYNCCH1_PORTB_PIN2_gc;      // Use CCL LUT1 as event generator
  EVSYS.ASYNCUSER3 = EVSYS_ASYNCUSER3_ASYNCCH1_gc;  // ASYNCUSER4 is LUT1 event 0
   
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

 // Comparator0.hysteresis = comparator_hyst::large;      // Use positive input 2 (PB1)
 // Comparator1.hysteresis = comparator_hyst::large;      // Use positive input 3 (PB4)
 // Comparator2.hysteresis = comparator_hyst::large;      // Use positive input 1 (PB0)  

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
  //Logic0.filter = logic_filter::filter;        // No output filter enabled
  Logic0.sequencer = logic_sequencer::rs_latch; // Latch output
  Logic0.truth = 0xFE;                    // Set truth table (3 input OR)

  // Initialize logic block 0
  Logic0.init();

  // Start comparators
  Comparator0.start();
  Comparator1.start();
  Comparator2.start();
  
  // Start the AVR logic hardware
  Logic::start();


  


  
//  pinMode(LED, OUTPUT); // SUPER bright with this set.
//  SPI.swap(1); // Use alternate SPI pins
//  // wait for sd card
//  
//  while (!SD.begin(chipSelect)){}
//  Serial.begin(1000000);
//  while (!Serial) {}
//  UART.setSerialPort(&Serial); 
//
//  // create new file each boot
//  byte boots =  EEPROM.read(0);
//  EEPROM.update(0, ++boots);
//  // Filename must conform to short DOS 8.3
//  char fileName[12];
//  char temp[5];
//  strcpy(fileName, itoa(boots, temp, 10)); 
//  strcat(fileName, ".csv");
//  logFile = SD.open(fileName, FILE_WRITE);  
//
//  // Read firmware info and put at start of log file
//  while(!UART.getVescFirmwareInfo()){}
//  if ( UART.getVescFirmwareInfo() ) {
//    if (logFile) {
//      logFile.print("FW:");
//      logFile.print(UART.firmware.firmwareVersionMajor);
//      logFile.print(".");
//      logFile.print(UART.firmware.firmwareVersionMinor);
//      logFile.print(" B:");
//      logFile.println(UART.firmware.firmwareVersionBeta);
//      logFile.print("HW:");
//      logFile.println(UART.firmware.hardwareName);
//    }
//  }
  
 // Not required
 // logFile.println("ms_today,input_voltage,temp_mos_max,temp_mos_1,temp_mos_2,temp_mos_3,temp_motor,current_motor,current_in,d_axis_current,q_axis_current,erpm,duty_cycle,amp_hours_used,amp_hours_charged,watt_hours_used,watt_hours_charged,tachometer,tachometer_abs,encoder_position,fault_code,vesc_id,d_axis_voltage,q_axis_voltage");
}

int i = 0;
bool ledState = false;

void loop() {  
//    if(i > 10) {
//      digitalWrite(LED, ledState=!ledState);
//      i = 0;
//    }
//    if ( UART.getVescValues() ) {        
//        logFile.print(millis()); // Timestamp
//        logFile.print(",");
//        logFile.print(UART.data.input_voltage);
//        logFile.print(",");
//        logFile.print(UART.data.temp_mos_max);
//        logFile.print(",");
//        logFile.print(UART.data.temp_mos_1);
//        logFile.print(",");
//        logFile.print(UART.data.temp_mos_2);
//        logFile.print(",");
//        logFile.print(UART.data.temp_mos_3);
//        logFile.print(",");
//        logFile.print(UART.data.temp_motor);
//        logFile.print(",");
//        logFile.print(UART.data.current_motor);
//        logFile.print(",");
//        logFile.print(UART.data.current_in);
//        logFile.print(",");
//        logFile.print(UART.data.d_axis_current);
//        logFile.print(",");
//        logFile.print(UART.data.q_axis_current);
//        logFile.print(",");
//        logFile.print(UART.data.erpm);
//        logFile.print(",");
//        logFile.print(UART.data.duty_cycle);
//        logFile.print(",");
//        logFile.print(UART.data.amp_hours_used);
//        logFile.print(",");
//        logFile.print(UART.data.amp_hours_charged);
//        logFile.print(",,,,,");      
//        logFile.print(UART.data.encoder_position);
//        logFile.print(",");
//        logFile.print(UART.data.fault_code);
//        logFile.print(",");
//        logFile.print(UART.data.vesc_id);
//        logFile.print(",");
//        logFile.print(UART.data.d_axis_voltage);
//        logFile.print(",");
//        logFile.println(UART.data.q_axis_voltage);
//        logFile.availableForWrite();
//        i++;       
//    } else {
//        digitalWrite(LED, LOW);  
//    }    
}
