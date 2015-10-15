#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <Wire.h>
#include <string.h>
#include <SoftwareSerial.h>

#define LOG_INTERVAL  5000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 5000 // mills between calls to flush() - to write data to the card
#define ECHO_TO_SERIAL   1 // echo data to serial port

RTC_DS1307 RTC;
char FileName[] = "00.csv"; //File name is day-hour minute
char FolderName[] = "00-00-00"; //Folder is year-month
char file[8]; //File name character storage
char folder[10]; //Folder name character storage
char location[20]; //location for file to be created
File logFile;
SoftwareSerial SPBSerial(A3, A2);
uint32_t syncTime = 0; // time of last sync()
String inputString = "";

void setup() {
  Serial.begin(115200);
  SPBSerial.begin(2400);
  inputString.reserve(100);
  Wire.begin(); //Important for RTClib.h
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    return;
  }
  Serial.print("Initializing SD card..."); 
  pinMode(10, OUTPUT);
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
} 

void loop() {
delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  getFolderName();
  createFolder();
  getFileName();
  createFile();
  
  writeTime();
  writeSPBMode();
  writeSPBGenerealParams();
  writeSPBWarnings();
  writeSPBErrors();
  logFile.println();
#if ECHO_TO_SERIAL
  Serial.println();  
#endif //ECHO_TO_SERIAL
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logFile.flush();
  logFile.close();
  
}

void getFolderName() {
  DateTime now = RTC.now();
  FolderName[0] = (now.year()/10)%10 + '0'; //To get 3rd digit from year()
  FolderName[1] = now.year()%10 + '0'; //To get 4th digit from year()
  FolderName[3] = now.month()/10 + '0'; //To get 1st digit from month()
  FolderName[4] = now.month()%10 + '0'; //To get 2nd digit from month()\
  FolderName[6] = now.day()/10 + '0'; //To get 1st digit from day()
  FolderName[7] = now.day()%10 + '0'; //To get 2nd digit from day()
}

void createFolder() {
  Serial.print(FolderName);
  if (SD.exists(FolderName)) {
    Serial.println(" exists.");
  }
  else {
    Serial.print(" doesn't exist.");
    Serial.print(" Creating new folder ");
    Serial.println(FolderName);
    SD.mkdir(FolderName);
   } 
}

void getFileName(){
  DateTime now = RTC.now();
  FileName[0] = now.hour()/10 + '0'; //To get 1st digit from hour()
  FileName[1] = now.hour()%10 + '0'; //To get 2nd digit from hour()
}

void createFile(){
//Check file name exist?
  strcpy(file,FileName);
  strcpy(folder,FolderName);
  strcpy(location,"/");
  strcat(location, folder);
  strcat(location, "/");
  strcat(location, file);
  Serial.print("Location: ");
  Serial.print(location);
  if (SD.exists(location)) {
    Serial.println(" exists.");
    logFile = SD.open(location, FILE_WRITE);
  }
  else {
    Serial.print(" doesn't exist. ");
    Serial.print("Creating new file ");
    Serial.println(FileName);
    logFile = SD.open(location, FILE_WRITE);
  } 
}

void writeTime() {
  DateTime now = RTC.now();
  logFile.print(now.year(), DEC);
  logFile.print("/");
  logFile.print(now.month(), DEC);
  logFile.print("/");
  logFile.print(now.day(), DEC);
  logFile.print(" ");
  logFile.print(now.hour(), DEC);
  logFile.print(":");
  logFile.print(now.minute(), DEC);
  logFile.print(":");
  logFile.print(now.second(), DEC);
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
//    Serial.print(ERR);
  } else if (inputString.indexOf("(") != 0) {
//    Serial.print(ERR);
  } else {
     inputString = inputString.substring(1);
  }
}

void writeSPBMode() {
  SPBSerial.print("QMOD \r");
  delay(50);

  SPBserialEvent();
  validateInStr(3);
  logFile.print(",");    
  logFile.print(inputString.substring(0,1));
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
  
  logFile.print(","); 
  logFile.print(inputString.substring(0,5));logFile.print(",");
  logFile.print(inputString.substring(6,10));logFile.print(",");
  logFile.print(inputString.substring(11, 16));logFile.print(",");
  logFile.print(inputString.substring(17,21));logFile.print(",");
  logFile.print(inputString.substring(22,27));logFile.print(",");
  logFile.print(inputString.substring(28,31));logFile.print(",");
  logFile.print(inputString.substring(32,37));logFile.print(",");
  logFile.print(inputString.substring(38,43));logFile.print(",");
  logFile.print(inputString.substring(44,49));logFile.print(",");
  logFile.print(inputString.substring(56,61));
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

  logFile.print(","); 
  logFile.print(inputString);
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

  logFile.print(","); 
  logFile.print(inputString);
#if ECHO_TO_SERIAL
  Serial.print(","); 
  Serial.print(inputString);
#endif //ECHO_TO_SERIAL
  inputString = "";
}
