/*
  SD card datalogger

  created  23 Feb 2022
  modified 11 Apr 2022
  by Euan Mutch (TechAUmNu)
  
*/


#include <SPI.h>
#include <SD.h>
#include "VescUart.h"
const int LED = 4;
const int chipSelect = 0;

VescUart UART;
File dataFile;

void setup() {
  pinMode(LED, OUTPUT);

  digitalWrite(LED, HIGH);  
  delay(2000);                       
  digitalWrite(LED, LOW); 
   
  // see if the card is present and can be initialized:

   while (!SD.begin(chipSelect))
   {      
      digitalWrite(LED, HIGH);  
      delay(100);                       
      digitalWrite(LED, LOW);    
      delay(100);
      digitalWrite(LED, HIGH);  
      delay(100);                       
      digitalWrite(LED, LOW);    
      delay(100);   
      digitalWrite(LED, HIGH);  
      delay(100);                       
      digitalWrite(LED, LOW);    
      delay(100);
  } 

 dataFile = SD.open("datalog.txt", FILE_WRITE);

  // Once SDCard is ok, connect to VESC
   Serial.begin(115200);
   while (!Serial) {;}
   UART.setSerialPort(&Serial);  

// Read firmware info and log
if ( UART.getVescFirmwareInfo() ) {
    if (dataFile) {
      dataFile.println("VESC LOGGER by TechAUmNu on the VESC Discord, support here https://discord.com/channels/904830990319485030/937343467984662559");
      dataFile.print("FW: ");
      dataFile.print(UART.firmware.firmwareVersionMajor);
      dataFile.print(".");
      dataFile.print(UART.firmware.firmwareVersionMinor);
      dataFile.print("\tBeta: ");
      dataFile.println(UART.firmware.firmwareVersionBeta);
      dataFile.print("Hardware: ");
      dataFile.println(UART.firmware.hardwareName);
      dataFile.println("tempmos, tempmot, avgMotorCurrent, avgInputCurrent, dutyCycle, RPM, inputVoltage, ampHours, ampHoursCharged, wattHours, wattHoursCharged, tachometer, tachometerAbs, error, pidPos, id");    
    }  
  }
  else
  {

    dataFile.println("Failed to read firmware data");
  }
  
}

void loop() {
  // if the file is available, write to it:
  if (dataFile) {
      if ( UART.getVescValues() ) {
        digitalWrite(LED, HIGH);
        dataFile.print(UART.data.tempMosfet);
        dataFile.print(",\t");
        dataFile.print(UART.data.tempMotor);
        dataFile.print(",\t");
        dataFile.print(UART.data.avgMotorCurrent);
        dataFile.print(",\t");
        dataFile.print(UART.data.avgInputCurrent);
        dataFile.print(",\t");
        dataFile.print(UART.data.dutyCycleNow);
        dataFile.print(",\t");
        dataFile.print(UART.data.rpm);
        dataFile.print(",\t");
        dataFile.print(UART.data.inpVoltage);
        dataFile.print(",\t");
        dataFile.print(UART.data.ampHours);
        dataFile.print(",\t");
        dataFile.print(UART.data.ampHoursCharged);
        dataFile.print(",\t");
        dataFile.print(UART.data.wattHours);
        dataFile.print(",\t");
        dataFile.print(UART.data.wattHoursCharged);
        dataFile.print(",\t");
        dataFile.print(UART.data.tachometer);
        dataFile.print(",\t");
        dataFile.print(UART.data.tachometerAbs);
        dataFile.print(",\t");
        dataFile.print(UART.data.error);
        dataFile.print(",\t");
        dataFile.println(UART.data.pidPos);             
        dataFile.close();        
      }
      else
      {
        digitalWrite(LED, LOW); 
        dataFile.println("Failed to get data!");
      }
        
   
  }
  // if the file isn't open, probably means the card is not inserted
  else {    
     while (!SD.begin(chipSelect))
     {      
        digitalWrite(LED, HIGH);  
        delay(100);                       
        digitalWrite(LED, LOW);    
        delay(100);
        digitalWrite(LED, HIGH);  
        delay(100);                       
        digitalWrite(LED, LOW);    
        delay(100);   
        digitalWrite(LED, HIGH);  
        delay(100);                       
        digitalWrite(LED, LOW);    
        delay(100);
    }
    dataFile = SD.open("datalog.txt", FILE_WRITE);    
  }
}
