#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <Wire.h>
#include <string.h>
#include <SoftwareSerial.h>

#define LOG_INTERVAL  5000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 5000 // mills between calls to flush() - to write data to the card
#define ECHO_TO_SERIAL   1 // Дублирование информации в Serial порт (вкл/выкл)
#define ERR_LED_PIN      111 //Пин светодиода

RTC_DS1307 RTC; //Объект для работы с часами реального времени
File logFile;   //Объект для работы с файлом
SoftwareSerial SPBSerial(A3, A2); //Объект для работы с софтверным Serial портом

char FileName[]   = "00.csv";   //Имя файла (номер часа)
char FolderName[] = "00-00-00"; //Имя папки (гг-мм-дд)
char file[8];                   //Буфер для имени файла
char folder[10];                //Буфер для имени папки
char location[22];              //Буфер для хранение целого пути до файла


uint32_t syncTime = 0; // time of last sync()
String inputString = "";
boolean err = false;

void setup() {
  Serial.begin(115200);
  SPBSerial.begin(2400);
  inputString.reserve(100);

  pinMode(ERR_LED_PIN, OUTPUT);
  digitalWrite(ERR_LED_PIN, LOW);

  //Инициализация RTC
  Wire.begin();
  RTC.begin();
  if (!RTC.isrunning()) {
    Serial.println("RTCe");
    err = true;
    return;
  }
  //RTC.adjust(DateTime(__DATE__, __TIME__));

  //Инициализация SD
  pinMode(10, OUTPUT);
  if (!SD.begin(10)) {
    Serial.println("SDe");
    err = true;
    return;
  }
} 

void loop() {
  if (err) {
    digitalWrite(ERR_LED_PIN, HIGH);
  }
  
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

  sdSync();
}

void sdSync() {
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
  if (SD.exists(location)) {
    Serial.print(location);
    Serial.println(" exists.");
    logFile = SD.open(location, FILE_WRITE);
  }
  else {
    Serial.print("Creating new file ");
    Serial.println(location);
    logFile = SD.open(location, FILE_WRITE);
    logFile.print("Date,Time,Mode,V_in,F_in,V_out,F_out,I_out,Load,Pos_DC,Neg_DC,V_bat,Temp,");
    Serial.print("Date,Time,mode,V_in,F_in,V_out,F_out,I_out,Load,Pos_DC,Neg_DC,V_bat,Temp,");
    for (int i = 0; i < 56; i++) {
      logFile.print("a");
      logFile.print(i);
      logFile.print(",");
      Serial.print("a");
      Serial.print(i);
      Serial.print(",");
    }
    logFile.println("Error");
    Serial.println("Error");
  } 
}

void writeTime() {
  DateTime now = RTC.now();
  logFile.print(now.year(), DEC);
  logFile.print("/");
  
  logFile.print(now.month(), DEC);
  byte month = (byte)now.month();
  String monthStr = String(month);
  if (month < 10) {
    monthStr = "0" + String(month);
  }
  logFile.print("/");

  byte day = (byte)now.day();
  String dayStr = String(day);
  if (day < 10) {
    dayStr = "0" + String(day);
  }
  logFile.print(dayStr);
  logFile.print(",");

  byte hour = (byte)now.hour();
  String hourStr = String(hour);
  if (hour < 10) {
    hourStr = "0" + String(hour);
  }
  logFile.print(hourStr);
  logFile.print(":");

  byte minute = (byte)now.minute();
  String minuteStr = String(minute);
  if (minute < 10) {
    minuteStr = "0" + String(minute);
  }
  logFile.print(minuteStr);
  logFile.print(":");

  byte second = (byte)now.second();
  String secondStr = String(second);
  if (second < 10) {
    secondStr = "0" + String(second);
  }
  logFile.print(secondStr);
#if ECHO_TO_SERIAL
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(monthStr);
  Serial.print("/");
  Serial.print(dayStr);
  Serial.print(",");
  Serial.print(hourStr);
  Serial.print(":");
  Serial.print(minuteStr);
  Serial.print(":");
  Serial.print(secondStr);
#endif //ECHO_TO_SERIAL
}

/**
 * Метод, обрабатывающий события программного Serial порта.
 * Сохраняет все полученные данные в строку inputString.
 */
void SPBserialEvent() {
  while (SPBSerial.available()) {
    char inChar = (char)SPBSerial.read();
    inputString += inChar;
  }
}

/**
 * Метод, валидирующий полученную в методе SPBserialEvent строку.
 * Валидация проходит по длине строки и наличию стартового символа "("
 * Если строка не прошла валидацию, то она очищается
 */
boolean validateInStr(int strLength) {
  boolean res = true;
  if (inputString.length() != strLength) {
    inputString = "";
    res = false;
  } else if (inputString.indexOf("(") != 0) {
    inputString = "";
    res = false;
  } else {
     inputString = inputString.substring(1);
  }
  return res;
}

/**
 * Метод, записывающий режим работы ИБП
 */
void writeSPBMode() {
  SPBSerial.print("QMOD \r");
  delay(50);

  SPBserialEvent();
  boolean validate = validateInStr(3);

  logFile.print(",");
  if (validate) {
    logFile.print(inputString.substring(0,1));  
    err = false;
  } else {
    err = true;
  }
  
  
#if ECHO_TO_SERIAL
  Serial.print(",");  
  if (validate) {
    Serial.print(inputString.substring(0,1));  
  }  
#endif //ECHO_TO_SERIAL

  inputString = "";
}

void writeSPBGenerealParams() {
  SPBSerial.print("QGS \r");
  delay(200);
  
  SPBserialEvent();
  boolean validate = validateInStr(63);
  
  logFile.print(","); 
  if (validate) {
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

    err = false;
  } else {
    err = true;
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
    logFile.print(",");
  }
  
#if ECHO_TO_SERIAL
  Serial.print(","); 
  if (validate) {
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
  } else {
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
    Serial.print(",");
  }
  
#endif //ECHO_TO_SERIAL
  inputString = "";
}

void writeSPBWarnings() {
  SPBSerial.print("QWS \r");
  delay(200);
  

  SPBserialEvent();
  boolean validate = validateInStr(63);
  
  logFile.print(","); 
  Serial.print(",");
  int warningCount = 56;

  if (validate) {
    err = false;
    for (int i = 0; i < warningCount; i++) {
      logFile.print(inputString.substring(i, i+1)); 
      Serial.print(inputString.substring(i, i+1)); 
      logFile.print(",");
      Serial.print(",");
    }
  } else {
    err = true;
    for (int i = 0; i < warningCount; i++) {
      logFile.print(",");
      Serial.print(",");
    }
  }

  inputString = "";
}

void writeSPBErrors() {
  SPBSerial.print("QFS \r");
  delay(200);

  SPBserialEvent();
  boolean validate = validateInStr(63);

  if (validate) {
    err = false;
    logFile.print(inputString);
    Serial.print(inputString);
  } else {
    err = true;
  }  
  inputString = "";
}
