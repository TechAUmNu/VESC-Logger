#ifndef _VESCUART_h
#define _VESCUART_h

#include <Arduino.h>
#include "datatypes.h"
#include "buffer.h"
#include "crc.h"

class VescUart
{


    /** Struct to store the telemetry data returned by the VESC */
    struct commValues {
      float input_voltage;
      float temp_mos_max;
      float temp_mos_1;
      float temp_mos_2;
      float temp_mos_3;
      float temp_motor;
      float current_motor;
      float current_in;
      float d_axis_current;
      float q_axis_current;
      float erpm;
      float duty_cycle;
      float amp_hours_used;
      float amp_hours_charged;
      float watt_hours_used;
      float watt_hours_charged;
      int32_t tachometer;
      int32_t tachometer_abs;
      float encoder_position;
      uint8_t fault_code;
      uint8_t vesc_id;
      float d_axis_voltage;
      float q_axis_voltage;
    };
	
	struct commSetupValues {
      float temp_mos_max;
      float temp_motor;
      float current_motor;
      float current_in;
      float duty_cycle;
      float erpm;
      float speed;
      float input_voltage;
      float battery_level;
      float ah_total;
      float ah_charge_total;
      float wh_total;
      float wh_charge_total;
      float distance;
      float distance_abs;
      float pid_position;
      uint8_t fault_code;
      uint8_t vesc_id;
      uint8_t num_vescs;
      float wh_battery_left;
      uint32_t odometer;
      uint32_t setupValTime;      
    };
	
	struct commIMUValues {
      float rpy0;
      float rpy1;
      float rpy2;
      float acc0;
      float acc1;
      float acc2;
      float gyro0;
      float gyro1;
      float gyro2;
      float mag0;
      float mag1;
      float mag2;
      float q0;
      float q1;
      float q2;
      float q3;
      uint8_t vesc_id;      
    };

    struct firmwarePackage {
      uint8_t	firmwareVersionMajor;
      uint8_t	firmwareVersionMinor;
      char hardwareName[40];
      uint8_t firmwareVersionBeta;
    };


  public:
    /**
       @brief      Class constructor
    */
    VescUart(void);

    /** Variabel to hold measurements returned from VESC */
    commValues dataValues;
	commSetupValues dataSetupValues;
	commIMUValues dataIMU;

    /** Variabel to hold firmware version values */
    firmwarePackage firmware;

    /**
       @brief      Set the serial port for uart communication
       @param      port  - Reference to Serial port (pointer)
    */
    void setSerialPort(UartClass* port);

    /**
       @brief      Sends a command to VESC and stores the returned data

       @return     True if successfull otherwise false
    */
    bool getVescValues(void);
    bool getVescSetupValues(void);
    bool getIMUValues(void);
    /**
       @brief      Sends a command to VESC and stores the returned data

       @return     True if successfull otherwise false
    */
    bool getVescFirmwareInfo(void);

  private:

    /** Variabel to hold the reference to the Serial object to use for UART */
    HardwareSerial* serialPort = NULL;


    /**
       @brief      Packs the payload and sends it over Serial

       @param      payload  - The payload as a unit8_t Array with length of int lenPayload
       @param      lenPay   - Length of payload
       @return     The number of bytes send
    */
    int packSendPayload(uint8_t * payload, int lenPay);

    /**
       @brief      Receives the message over Serial

       @param      payloadReceived  - The received payload as a unit8_t Array
       @return     The number of bytes receeived within the payload
    */
    int receiveUartMessage(uint8_t * payloadReceived);

    /**
       @brief      Verifies the message (CRC-16) and extracts the payload

       @param      message  - The received UART message
       @param      lenMes   - The lenght of the message
       @param      payload  - The final payload ready to extract data from
       @return     True if the process was a success
    */
    bool unpackPayload(uint8_t * message, int lenMes, uint8_t * payload);

    /**
       @brief      Extracts the data from the received payload

       @param      message  - The payload to extract data from
       @return     True if the process was a success
    */
    bool processReadPacket(uint8_t * message);


};

#endif
