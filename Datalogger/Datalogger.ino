/*
  SD card datalogger

  created  23 Feb 2022
  modified 11 Apr 2022
  by Euan Mutch (TechAUmNu)

*/


// TODO:
// Handling for removed/reinserted sd cards - requires switch on card socket to be wired up on pcb!
// Reclaim some space!

#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include "VescUart.h"

const int LED = 4;
const int chipSelect = 0;

VescUart UART;
File logFile, infoFile;

void setup() {
  pinMode(LED, OUTPUT); // SUPER bright with this set.
  
  // wait for sd card
  while (!SD.begin(chipSelect)){}
  Serial.begin(1000000);
  while (!Serial) {}
  UART.setSerialPort(&Serial); 

  // create new file each boot
  byte boots =  EEPROM.read(0);
  EEPROM.update(0, ++boots);
  // Filename must conform to short DOS 8.3
  char fileName[12];
  char temp[5];
  strcpy(fileName, itoa(boots, temp, 10)); 
  strcat(fileName, ".csv");
  logFile = SD.open(fileName, FILE_WRITE);  

  // Read firmware info and put at start of log file
  while(!UART.getVescFirmwareInfo()){}
  if ( UART.getVescFirmwareInfo() ) {
    if (logFile) {
      logFile.print("FW:");
      logFile.print(UART.firmware.firmwareVersionMajor);
      logFile.print(".");
      logFile.print(UART.firmware.firmwareVersionMinor);
      logFile.print(" B:");
      logFile.println(UART.firmware.firmwareVersionBeta);
      logFile.print("HW:");
      logFile.println(UART.firmware.hardwareName);
    }
  }
  
 // Not required
 // logFile.println("ms_today,input_voltage,temp_mos_max,temp_mos_1,temp_mos_2,temp_mos_3,temp_motor,current_motor,current_in,d_axis_current,q_axis_current,erpm,duty_cycle,amp_hours_used,amp_hours_charged,watt_hours_used,watt_hours_charged,tachometer,tachometer_abs,encoder_position,fault_code,vesc_id,d_axis_voltage,q_axis_voltage");
}

int i = 0;
bool ledState = false;

void loop() {  
    if(i > 10) {
      digitalWrite(LED, ledState=!ledState);
      i = 0;
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
        i++;       
    } else {
        digitalWrite(LED, LOW);  
    }    
}
