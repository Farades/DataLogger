#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

#define ERR "err"

#define LOG_INTERVAL  5000 // mills between entries (reduce to take more/faster data)

#define SYNC_INTERVAL 5000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port

RTC_DS1307 RTC; // define the Real Time Clock object

/**
 * Serial порт для работы с SPB. A3 - RX, A2 - TX.
 */
SoftwareSerial SPBSerial(A3, A2);

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

String inputString = "";

void setup(void)
{
  Serial.begin(9600);
  inputString.reserve(100);
  SPBSerial.begin(2400);

  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println(ERR);
  }
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (!logfile) {
    Serial.println(ERR);
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();  
  RTC.begin();
  
  logfile.println("time,mode,V_in,F_in,V_out,F_out,I_out,Load,POS_DC,NEG_DC,V_Bat,Temp,Warnings,Errors");    
#if ECHO_TO_SERIAL
  Serial.println("time,mode,V_in,F_in,V_out,F_out,I_out,Load,POS_DC,NEG_DC,V_Bat,Temp,Warnings,Errors");   
#endif //ECHO_TO_SERIAL
}

void loop(void)
{
  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  test();
  writeTime();
  writeSPBMode();
  writeSPBGenerealParams();
  writeSPBWarnings();
  writeSPBErrors();

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();  
#endif //ECHO_TO_SERIAL
  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();

  logfile.flush();
}

void writeTime() {
  DateTime now = RTC.now();
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
#if ECHO_TO_SERIAL
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
#endif //ECHO_TO_SERIAL
}

void SPBserialEvent() {
  while (SPBSerial.available()) {
    char inChar = (char)SPBSerial.read();
    inputString += inChar;
  }
}

void validateInStr(int strLength) {
  if (inputString.length() != strLength) {
    Serial.print(ERR);
  } else if (inputString.indexOf("(") != 0) {
    Serial.print(ERR);
  } else {
     inputString = inputString.substring(1);
  }
}

void test() {
  SPBSerial.print("TL \r");
  delay(50);
  SPBserialEvent();
  Serial.println(inputString);
  inputString = "";
}

void writeSPBMode() {
  SPBSerial.print("QMOD \r");
  delay(50);

  SPBserialEvent();
  validateInStr(3);
  logfile.print(",");    
  logfile.print(inputString.substring(0,1));
#if ECHO_TO_SERIAL
  Serial.print(",");    
  Serial.print(inputString.substring(0,1));
#endif //ECHO_TO_SERIAL
  inputString = "";
}

void writeSPBGenerealParams() {
  SPBSerial.print("QGS \r");
  delay(200);
  
  SPBserialEvent();
  validateInStr(63);
  
  logfile.print(","); 
  logfile.print(inputString.substring(0,5));logfile.print(",");
  logfile.print(inputString.substring(6,10));logfile.print(",");
  logfile.print(inputString.substring(11, 16));logfile.print(",");
  logfile.print(inputString.substring(17,21));logfile.print(",");
  logfile.print(inputString.substring(22,27));logfile.print(",");
  logfile.print(inputString.substring(28,31));logfile.print(",");
  logfile.print(inputString.substring(32,37));logfile.print(",");
  logfile.print(inputString.substring(38,43));logfile.print(",");
  logfile.print(inputString.substring(44,49));logfile.print(",");
  logfile.print(inputString.substring(56,61));
#if ECHO_TO_SERIAL
  Serial.print(","); 
  Serial.print(inputString.substring(0,5));Serial.print(","); 
  Serial.print(inputString.substring(6,10));Serial.print(","); 
  Serial.print(inputString.substring(11, 16));Serial.print(","); 
  Serial.print(inputString.substring(17,21));Serial.print(","); 
  Serial.print(inputString.substring(22,27));Serial.print(","); 
  Serial.print(inputString.substring(28,31));Serial.print(","); 
  Serial.print(inputString.substring(32,37));Serial.print(","); 
  Serial.print(inputString.substring(38,43));Serial.print(","); 
  Serial.print(inputString.substring(44,49));Serial.print(","); 
  Serial.print(inputString.substring(56,61));
#endif //ECHO_TO_SERIAL
  inputString = "";
}

void writeSPBWarnings() {
  SPBSerial.print("QWS \r");
  delay(200);

  SPBserialEvent();
  validateInStr(63);

  logfile.print(","); 
  logfile.print(inputString);
#if ECHO_TO_SERIAL
  Serial.print(","); 
  Serial.print(inputString);
#endif //ECHO_TO_SERIAL
  inputString = "";
}

void writeSPBErrors() {
  SPBSerial.print("QFS \r");
  delay(200);

  SPBserialEvent();
  validateInStr(63);

  logfile.print(","); 
  logfile.print(inputString);
#if ECHO_TO_SERIAL
  Serial.print(","); 
  Serial.print(inputString);
#endif //ECHO_TO_SERIAL
  inputString = "";
}
