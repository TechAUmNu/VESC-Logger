#ifndef _VESCUART_h
#define _VESCUART_h

#include <Arduino.h>
#include "datatypes.h"
#include "buffer.h"
#include "crc.h"

class VescUart
{
	/** Struct to store the telemetry data returned by the VESC */
	struct dataPackage {
		float avgMotorCurrent;
		float avgInputCurrent;
		float dutyCycleNow;
		float rpm;
		float inpVoltage;
		float ampHours;
		float ampHoursCharged;
		float wattHours;
		float wattHoursCharged;
		long tachometer;
		long tachometerAbs;
		float tempMosfet;
		float tempMotor;
		uint8_t error; 
		float pidPos;
		uint8_t id; 
	};

	/** Struct to hold the nunchuck values to send over UART */
	struct firmwarePackage {
		uint8_t	firmwareVersionMajor;
		uint8_t	firmwareVersionMinor;
		char hardwareName[40]; // valUpperButton
		uint8_t firmwareVersionBeta;
	};


	public:
		/**
		 * @brief      Class constructor
		 */
		VescUart(void);

		/** Variabel to hold measurements returned from VESC */
		dataPackage data; 

		/** Variabel to hold firmware version values */
		firmwarePackage firmware; 

		/**
		 * @brief      Set the serial port for uart communication
		 * @param      port  - Reference to Serial port (pointer) 
		 */
		void setSerialPort(UartClass* port);
	

		/**
		 * @brief      Sends a command to VESC and stores the returned data
		 *
		 * @return     True if successfull otherwise false
		 */
		bool getVescValues(void);
		
		/**
		 * @brief      Sends a command to VESC and stores the returned data
		 *
		 * @return     True if successfull otherwise false
		 */
		bool getVescFirmwareInfo(void);
		
	private: 

		/** Variabel to hold the reference to the Serial object to use for UART */
		HardwareSerial* serialPort = NULL;

		/** Variabel to hold the reference to the Serial object to use for debugging. 
		  * Uses the class Stream instead of HarwareSerial */
		Stream* debugPort = NULL;

		/**
		 * @brief      Packs the payload and sends it over Serial
		 *
		 * @param      payload  - The payload as a unit8_t Array with length of int lenPayload
		 * @param      lenPay   - Length of payload
		 * @return     The number of bytes send
		 */
		int packSendPayload(uint8_t * payload, int lenPay);

		/**
		 * @brief      Receives the message over Serial
		 *
		 * @param      payloadReceived  - The received payload as a unit8_t Array
		 * @return     The number of bytes receeived within the payload
		 */
		int receiveUartMessage(uint8_t * payloadReceived);

		/**
		 * @brief      Verifies the message (CRC-16) and extracts the payload
		 *
		 * @param      message  - The received UART message
		 * @param      lenMes   - The lenght of the message
		 * @param      payload  - The final payload ready to extract data from
		 * @return     True if the process was a success
		 */
		bool unpackPayload(uint8_t * message, int lenMes, uint8_t * payload);

		/**
		 * @brief      Extracts the data from the received payload
		 *
		 * @param      message  - The payload to extract data from
		 * @return     True if the process was a success
		 */
		bool processReadPacket(uint8_t * message);

};

#endif
