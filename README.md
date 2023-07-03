# VESC-Logger

Datalogger:
A logger for VESC firmware designed to run on an ATTiny3216, which allows saving data to an SD card.
This is integrated directly into the A200S V4.1 logic board and communicates at 1Mbaud with the VESC.

Hardware Protection:
Controls low voltage outputs on A200S V4.1 logic board and runs on an ATTiny1616 (3.3v, 5v and 12v outputs). Efuses protect the outputs, after some time of continuous fault the output is turned off to reduce heat buildup. 

Hardware Overcurrent:
Both ATTiny chips are used for Overcurrent protection. They each have 3 comparators and a latch which makes them ideal for this.
The ATTiny3216 protects the Low side mosfet channel, and the ATTiny1616 protects the diode, this also indirectly protects the high side channel as well.
The trip voltage is supplied by Digipots that are configured by the STM32.

At startup the digipots are both volatile so default to midrail, 1.65v, corresponding to 0A. So the overcurrent trips on both ATTiny and is latched.
The STM32 then reads back a handshake code from each of the ATTiny over I2C, sets the digipots to the correct values (using I2C), and sends an I2C command to reset the latch.
Now that the digipot has the correct voltage the comparators are not triggered and the latch is cleared so the gate drivers can be enabled by the STM32 pulling down the disable pin (The pulldown path loops through the connector between the boards so it is not possible to enable the gate drivers unless the connector is properly inserted)



Communication based on https://github.com/SolidGeek/VescUart
