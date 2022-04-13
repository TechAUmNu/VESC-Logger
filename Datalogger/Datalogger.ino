/*
  SD card datalogger

  created  23 Feb 2022
  modified 11 Apr 2022
  by Euan Mutch (TechAUmNu)

*/


// TODO:
// Handling for removed/reinserted sd cards
// Only write header in file one time?
// Create new file each time power up so the logs don't get muddled up
// Reclaim some space!

#include <SPI.h>
#include <SD.h>
#include "VescUart.h"
const int LED = 4;
const int chipSelect = 0;

VescUart UART;
File logFile, infoFile;

void setup() {
  pinMode(LED, OUTPUT);
  
  // see if the card is present and can be initialized:

  while (!SD.begin(chipSelect)){}

  logFile = SD.open("log.csv", FILE_WRITE);
  infoFile = SD.open("fw.txt", FILE_WRITE);
  
  // Once SDCard is ok, connect to VESC
  Serial.begin(1000000);
  while (!Serial) {}
  
  UART.setSerialPort(&Serial);

  // Read firmware info and log
  while(!UART.getVescFirmwareInfo()){}
  if ( UART.getVescFirmwareInfo() ) {
    if (infoFile) {
      infoFile.print("FW:");
      infoFile.print(UART.firmware.firmwareVersionMajor);
      infoFile.print(".");
      infoFile.print(UART.firmware.firmwareVersionMinor);
      infoFile.print(" B:");
      infoFile.println(UART.firmware.firmwareVersionBeta);
      infoFile.print("HW:");
      infoFile.println(UART.firmware.hardwareName);
      infoFile.close();
    }
  }
 
  logFile.println("ms_today,input_voltage,temp_mos_max,temp_mos_1,temp_mos_2,temp_mos_3,temp_motor,current_motor,current_in,d_axis_current,q_axis_current,erpm,duty_cycle,encoder_position,fault_code,vesc_id,d_axis_voltage,q_axis_voltage");
  digitalWrite(LED, HIGH);
}

void loop() {  
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
        digitalWrite(LED, HIGH);       
    } else {
      digitalWrite(LED, LOW);  
    }    
}
