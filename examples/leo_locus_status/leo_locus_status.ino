// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code turns on the LOCUS built-in datalogger. The datalogger
// turns off when power is lost, so you MUST turn it on every time
// you want to use it!
//
// Tested and works great with the Adafruit Ultimate GPS module
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/746
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada

//This code is intended for use with Arduino Leonardo and other ATmega32U4-based Arduinos

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// If using software serial (sketch example default):
//   Connect the GPS TX (transmit) pin to Digital 8
//   Connect the GPS RX (receive) pin to Digital 7
// If using hardware serial:
//   Connect the GPS TX (transmit) pin to Arduino RX1 (Digital 0)
//   Connect the GPS RX (receive) pin to matching TX1 (Digital 1)

// If using software serial, keep these lines enabled
// (you can change the pin numbers to match your wiring):
//SoftwareSerial mySerial(8, 7);
//Adafruit_GPS GPS(&mySerial);

// If using hardware serial, comment
// out the above two lines and enable these two lines instead:
Adafruit_GPS GPS(&Serial1);
HardwareSerial mySerial = Serial1;

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

void setup() {

  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  while (!Serial);  // the Leonardo will 'wait' until the USB plug is connected
  Serial.println("Adafruit GPS logging start test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);
  
  // You can adjust which sentences to have the module emit, below
  // Default is RMC + GGA
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Default is 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);

  while (true) {
    Serial.print("Starting logging....");
    if (GPS.LOCUS_StartLogger()) {
      Serial.println(" STARTED!");
      break;
    } else {
      Serial.println(" no response :(");
    }
  }
}

uint32_t updateTime = 1000;

void loop() {    // run over and over again

  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if ((c) && (GPSECHO))
    Serial.write(c); 
    
  if (millis() > updateTime) {
    updateTime = millis() + 1000;
    
    if (GPS.LOCUS_ReadStatus()) {
      Serial.print("\n\nLog #");
      Serial.print(GPS.LOCUS_GetSerial(), DEC);
       
      uint8_t type = GPS.LOCUS_GetType();
      if (type == LOCUS_OVERLAP)
        Serial.print(", Overlap, ");
      else if (type == LOCUS_FULLSTOP)
        Serial.print(", Full Stop, Logging");
      
      uint8_t mode = GPS.LOCUS_GetMode();
      if (mode & 0x1) Serial.print(" AlwaysLocate");
      if (mode & 0x2) Serial.print(" FixOnly");
      if (mode & 0x4) Serial.print(" Normal");
      if (mode & 0x8) Serial.print(" Interval");
      if (mode & 0x10) Serial.print(" Distance");
      if (mode & 0x20) Serial.print(" Speed");
      
      Serial.print(", Content "); Serial.print((int)GPS.LOCUS_GetConfig());
      Serial.print(", Interval "); Serial.print((int)GPS.LOCUS_GetInterval());
      Serial.print(" sec, Distance "); Serial.print((int)GPS.LOCUS_GetDistance());
      Serial.print(" m, Speed "); Serial.print((int)GPS.LOCUS_GetSpeed());
      Serial.print(" m/s, Status ");
      if (GPS.LOCUS_GetStatus())
        Serial.print("LOGGING, ");
      else
        Serial.print("OFF, ");
      Serial.print((int)GPS.LOCUS_GetRecords()); Serial.print(" Records, ");
      Serial.print((int)GPS.LOCUS_GetPercent()); Serial.print("% Used ");
  
    }  // if (GPS.LOCUS_ReadStatus())
  }  // if (millis() > updateTime)
}  // loop


/******************************************************************/
// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO && c) {
#ifdef UDR0
    UDR0 = c;  
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
#endif
  }
}


void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}
