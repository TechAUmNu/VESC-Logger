/*
 * 
 * Run at 8MHz to reduce chance of false trip on the OC
  created  23 Feb 2022
  modified 16 Apr 2023
  by Euan Mutch (TechAUmNu)

*/


#include <Comparator.h>
#include <Logic.h>
#include <Event.h>
#include <Wire.h>
#include <movingAvg.h>

#define wdt_reset() __asm__ __volatile__ ("wdr"::)
#define ATTINY1616_HANDSHAKE_REPLY 0x73

#define RESTORE_3V3_DELAY 1000
#define RESTORE_5V_DELAY 1000
#define RESTORE_12V_DELAY 20000

#define TRIP_3V3_DELAY 100
#define TRIP_5V_DELAY 100
#define TRIP_12V_DELAY 2000

#define STARTUP_12V_DELAY 5000

#define TRIP_12V_THRES 800 //11.3V

const int PIN_LED = 10;
const int PIN_3V3_EN = 7;
const int PIN_3V3_FAULT = 6;
const int PIN_5V_EN = 12;
const int PIN_5V_FAULT = 13;
const int PIN_P_GOOD = 11;
const int PIN_12V_AUX_MON = 4;
const int PIN_12V_AUX_EN = 16;

static int F_3V3_count = 0;
static int F_5V_count = 0;
static int F_12V_count = 0;
static int OK_12V_count = 0;
static int F_PGOOD_count = 0;
static int Startup_12V_Count = 0;
static bool AUX_FAULTED = false;

byte boots;
movingAvg V_12V_AUX_MON(10);



void setup() {

  _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_1KCLK_gc); //enable the WDT, 1s https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/Ref_Reset.md
  
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_3V3_EN, OUTPUT);
  pinMode(PIN_5V_EN, OUTPUT);
  pinMode(PIN_12V_AUX_EN, OUTPUT);
  pinMode(PIN_3V3_FAULT, INPUT_PULLUP);
  pinMode(PIN_5V_FAULT, INPUT_PULLUP);
  pinMode(PIN_P_GOOD, INPUT_PULLUP);
  pinMode(PIN_12V_AUX_MON, INPUT);

  digitalWrite(PIN_LED, 1);

  // Check the watchdog actually works
  //uint8_t resetflags = GPIOR0;
  //if (resetflags == RSTCTRL_WDRF_bm){
  //  while(true);
  //}

  // Configure event channel for reset  
  Event1.set_generator(0xFF);
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
  Logic0.filter = logic_filter::filter;
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
  Wire.begin(0x68);                 // join i2c bus with address 0x68
  Wire.onReceive(processCommand);
  Wire.onRequest(processHandshake);

  digitalWrite(PIN_LED, 0);

  // Turn on outputs that should be on all the time
  digitalWrite(PIN_3V3_EN, 1);
  digitalWrite(PIN_5V_EN, 1);
  digitalWrite(PIN_12V_AUX_EN, 0);
  V_12V_AUX_MON.begin();
}

static bool AUX_ENABLED = false;

void processCommand(int16_t numBytes) {
  switch(Wire.read()){
    case 0x54:
      Event1.soft_event(); 
      break;

    case 0x21:
      AUX_ENABLED = true;
      Startup_12V_Count = STARTUP_12V_DELAY; 
      break;

    case 0x20:
      AUX_ENABLED = false;
      break;
  } 
}

void processHandshake() { 
 Wire.write(ATTINY1616_HANDSHAKE_REPLY);  // Handshake to enable gate driver 
}

// Instantly trip outputs, retry after 1s.
void loop() {
  wdt_reset();
  delay(1);
  digitalWrite(PIN_LED, 0);
  if (!digitalRead(PIN_3V3_FAULT))
  {
    if (F_3V3_count < TRIP_3V3_DELAY)
    {
      F_3V3_count++;
    } else {
      digitalWrite(PIN_3V3_EN, 0);
      digitalWrite(PIN_LED, 1);
      F_3V3_count = RESTORE_3V3_DELAY;
    }
  } else {
    if (F_3V3_count > 0)
    {
      F_3V3_count--;
      digitalWrite(PIN_LED, 1);
    } else {
      digitalWrite(PIN_3V3_EN, 1);
    }
  }

  if (!digitalRead(PIN_5V_FAULT))
  {
    if (F_5V_count < TRIP_5V_DELAY)
    {
      F_5V_count++;
    } else {
      digitalWrite(PIN_5V_EN, 0);
      digitalWrite(PIN_LED, 1);
      F_5V_count = 1000;
    }

  } else {
    if (F_5V_count > 0)
    {
      F_5V_count--;
      digitalWrite(PIN_LED, 1);
    } else {
      digitalWrite(PIN_5V_EN, 1);
    }
  }

  if (AUX_ENABLED)
  {    
    // Only enable 12v aux if the output is enabled and 12V rail is OK.
    if (!digitalRead(PIN_P_GOOD))
    {
      digitalWrite(PIN_LED, 1);
      digitalWrite(PIN_12V_AUX_EN, 0);
      F_PGOOD_count = 50;
    } else {
      if (F_PGOOD_count > 0)
      {
        F_PGOOD_count--;
      } else {
        if (V_12V_AUX_MON.reading(analogRead(PIN_12V_AUX_MON)) < 850)
        {
          if (AUX_FAULTED)
          {
            if(Startup_12V_Count <= 0)
              digitalWrite(PIN_12V_AUX_EN, 0);
              digitalWrite(PIN_LED, 1);
            if (F_12V_count > 0)
            {
              F_12V_count--;
            } else {
              AUX_FAULTED = false;
              Startup_12V_Count = STARTUP_12V_DELAY; // Allow things to start up again
              digitalWrite(PIN_12V_AUX_EN, 1);
            }
          } else {
            if (F_12V_count < TRIP_12V_DELAY)
            {
              F_12V_count++;
            } else {
              F_12V_count = RESTORE_12V_DELAY;
              AUX_FAULTED = true;
              if(Startup_12V_Count <= 0)
                digitalWrite(PIN_12V_AUX_EN, 0);
                digitalWrite(PIN_LED, 1);
            }
          }
        } else {
          //Windup protection
          if (OK_12V_count < TRIP_12V_DELAY / 5)
          {
            OK_12V_count++;
          } else {
            OK_12V_count = 0;
            F_12V_count = 0;
          }
        }
      }
      //Handle startup, allows high current start for fans etc.
      if (Startup_12V_Count > 0)
      {
        Startup_12V_Count--;
        digitalWrite(PIN_12V_AUX_EN, 1);
      }
    }

    
  } else {
    digitalWrite(PIN_12V_AUX_EN, 0);    
  }
}
