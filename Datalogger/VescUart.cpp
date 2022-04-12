#include "VescUart.h"
#include <HardwareSerial.h>

VescUart::VescUart(void) {
}

void VescUart::setSerialPort(HardwareSerial* port)
{
  serialPort = port;
}

int VescUart::receiveUartMessage(uint8_t * payloadReceived) {

  // Messages <= 255 starts with "2", 2nd byte is length
  // Messages > 255 starts with "3" 2nd and 3rd byte is length combined with 1st >>8 and then &0xFF

  uint16_t counter = 0;
  uint16_t endMessage = 256;
  bool messageRead = false;
  uint8_t messageReceived[256];
  uint16_t lenPayload = 0;

  uint32_t timeout = millis() + 100; // Defining the timestamp for timeout (100ms before timeout)

  while ( millis() < timeout && messageRead == false) {

    while (serialPort->available()) {

      messageReceived[counter++] = serialPort->read();

      if (counter == 2) {

        switch (messageReceived[0])
        {
          case 2:
            endMessage = messageReceived[1] + 5; //Payload size + 2 for sice + 3 for SRC and End.
            lenPayload = messageReceived[1];
            break;
        }
      }

      if (counter >= sizeof(messageReceived)) {
        break;
      }

      if (counter == endMessage && messageReceived[endMessage - 1] == 3) {
        messageReceived[endMessage] = 0;
        messageRead = true;
        break; // Exit if end of message is reached, even if there is still more data in the buffer.
      }
    }
  }

  bool unpacked = false;

  if (messageRead) {
    unpacked = unpackPayload(messageReceived, endMessage, payloadReceived);
  }

  if (unpacked) {
    // Message was read
    return lenPayload;
  }
  else {
    // No Message Read
    return 0;
  }
}


bool VescUart::unpackPayload(uint8_t * message, int lenMes, uint8_t * payload) {

  uint16_t crcMessage = 0;
  uint16_t crcPayload = 0;

  // Rebuild crc:
  crcMessage = message[lenMes - 3] << 8;
  crcMessage &= 0xFF00;
  crcMessage += message[lenMes - 2];

  // Extract payload:
  memcpy(payload, &message[2], message[1]);

  crcPayload = crc16(payload, message[1]);

  if (crcPayload == crcMessage) {
    return true;
  } else {
    return false;
  }
}


int VescUart::packSendPayload(uint8_t * payload, int lenPay) {

  uint16_t crcPayload = crc16(payload, lenPay);
  int count = 0;
  uint8_t messageSend[256];

  if (lenPay <= 256)
  {
    messageSend[count++] = 2;
    messageSend[count++] = lenPay;
  }
  else
  {
    messageSend[count++] = 3;
    messageSend[count++] = (uint8_t)(lenPay >> 8);
    messageSend[count++] = (uint8_t)(lenPay & 0xFF);
  }

  memcpy(&messageSend[count], payload, lenPay);

  count += lenPay;
  messageSend[count++] = (uint8_t)(crcPayload >> 8);
  messageSend[count++] = (uint8_t)(crcPayload & 0xFF);
  messageSend[count++] = 3;
  messageSend[count] = '\0';

  // Sending package
  serialPort->write(messageSend, count);

  // Returns number of send bytes
  return count;
}


bool VescUart::processReadPacket(uint8_t * message) {

  COMM_PACKET_ID packetId;
  int32_t ind = 0;

  packetId = (COMM_PACKET_ID)message[0];
  message++; // Removes the packetId from the actual message (payload)

  switch (packetId) {
    case COMM_FW_VERSION:
      firmware.firmwareVersionMajor = message[ind++];
      firmware.firmwareVersionMinor = message[ind++];
      strcpy(firmware.hardwareName, (const char *)&message[ind]);
      ind += strlen(firmware.hardwareName) + 1;
      ind += 12;
      ind++; // skip pairing
      firmware.firmwareVersionBeta = message[ind++];
      return true;

    case COMM_GET_VALUES: // Structure defined here: https://github.com/vedderb/bldc/blob/master/commands.c#L362

      data.temp_mos_max 		    = buffer_get_float16(message, 10.0f, &ind); 	    // 2 bytes - mc_interface_temp_fet_filtered()
      data.temp_motor 			    = buffer_get_float16(message, 10.0f, &ind); 	    // 2 bytes - mc_interface_temp_motor_filtered()
      data.current_motor 	      = buffer_get_float32(message, 100.0f, &ind);      // 4 bytes - mc_interface_read_reset_avg_motor_current()
      data.current_in 	        = buffer_get_float32(message, 100.0f, &ind);      // 4 bytes - mc_interface_read_reset_avg_input_current()
      data.d_axis_current       = buffer_get_float32(message, 100.0f, &ind);;     // 4 bytes - mc_interface_read_reset_avg_id()
      data.q_axis_current       = buffer_get_float32(message, 100.0f, &ind);;     // 4 bytes - mc_interface_read_reset_avg_iq()
      data.duty_cycle 		      = buffer_get_float16(message, 1000.0f, &ind); 	  // 2 bytes - mc_interface_get_duty_cycle_now()
      data.erpm 				        = buffer_get_float32(message, 1.0f, &ind);		    // 4 bytes - mc_interface_get_rpm()
      data.input_voltage 		    = buffer_get_float16(message, 10.0f, &ind);		    // 2 bytes - GET_INPUT_VOLTAGE()
      data.amp_hours_used 			= buffer_get_float32(message, 10000.0f, &ind);	  // 4 bytes - mc_interface_get_amp_hours(false)
      data.amp_hours_charged 	  = buffer_get_float32(message, 10000.0f, &ind);	  // 4 bytes - mc_interface_get_amp_hours_charged(false)
      data.watt_hours_used			= buffer_get_float32(message, 10000.0f, &ind);	  // 4 bytes - mc_interface_get_watt_hours(false)
      data.watt_hours_charged 	= buffer_get_float32(message, 10000.0f, &ind);	  // 4 bytes - mc_interface_get_watt_hours_charged(false)
      data.tachometer 		      = buffer_get_int32(message, &ind);				        // 4 bytes - mc_interface_get_tachometer_value(false)
      data.tachometer_abs 		  = buffer_get_int32(message, &ind);				        // 4 bytes - mc_interface_get_tachometer_abs_value(false)
      data.fault_code 				  = message[ind++];								                  // 1 byte  - mc_interface_get_fault()
      data.encoder_position			= buffer_get_float32(message, 1000000.0f, &ind);	// 4 bytes - mc_interface_get_pid_pos_now()
      data.vesc_id					    = message[ind++];								                  // 1 byte  - app_get_configuration()->controller_id
      data.temp_mos_1           = buffer_get_float16(message, 10.0f, &ind);       // 2 bytes - NTC_TEMP_MOS1()
      data.temp_mos_2           = buffer_get_float16(message, 10.0f, &ind);       // 2 bytes - NTC_TEMP_MOS2()
      data.temp_mos_3           = buffer_get_float16(message, 10.0f, &ind);       // 2 bytes - NTC_TEMP_MOS3()
      data.d_axis_voltage       = buffer_get_float32(message, 1000.0f, &ind);;    // 4 bytes - mc_interface_read_reset_avg_vd()
      data.q_axis_voltage       = buffer_get_float32(message, 1000.0f, &ind);;    // 4 bytes - mc_interface_read_reset_avg_vq()
      return true;

    default:
      return false;
      break;
  }
}

bool VescUart::getVescFirmwareInfo(void) {

  uint8_t command[1] = { COMM_FW_VERSION };
  uint8_t payload[256];

  packSendPayload(command, 1);
  // delay(1); //needed, otherwise data is not read

  int lenPayload = receiveUartMessage(payload);

  if (lenPayload > 12) {
    bool read = processReadPacket(payload); //returns true if sucessful
    return read;
  }
  else
  {
    return false;
  }
}

bool VescUart::getVescValues(void) {

  uint8_t command[1] = { COMM_GET_VALUES };
  uint8_t payload[256];

  packSendPayload(command, 1);
  // delay(1); //needed, otherwise data is not read

  int lenPayload = receiveUartMessage(payload);

  if (lenPayload > 55) {
    bool read = processReadPacket(payload); //returns true if sucessful
    return read;
  }
  else
  {
    return false;
  }
}
