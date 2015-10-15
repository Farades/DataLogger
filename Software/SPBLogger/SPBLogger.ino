#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"
#include <Time.h>
#include <SPI.h>
#include <SD.h>

#define SPB_SERIAL_BAUDRATE    2400   // Скорость работы программного Serial-порта для связи с SPB
#define SPB_RESP_START_CHAR    "("    // Стартовый символ ответа SPB
#define SPB_SERIAL_BUFFER_SIZE 100    // Размер программного буфера
#define SPB_NOT_RESP           "NC"   // Сообщение, записываемое при отсутствии соединения с ИБП

const int chipSelect = 10;
File logfile;

/**
 * Serial порт для работы с SPB. A3 - RX, A2 - TX.
 */
SoftwareSerial SPBSerial(A3, A2);

/**
 * Программный буфер для SPBSerial
 */
char buffer[SPB_SERIAL_BUFFER_SIZE];

/**
 * Объект для работы с часами реального времени DS1307
 */
RTC_DS1307 RTC;

void setup() {
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();
  while (!Serial) {}
  SPBSerial.begin(SPB_SERIAL_BAUDRATE);

  pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("err");
  }
  Serial.println("card ok");
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
  if (! logfile) {
    Serial.println("file err");
  }
  Serial.print("Logging to: ");
  Serial.println(filename);
  logfile.println("time,mode,Vin"); 
}

/**
 * Статус опроса ИБП
 */
boolean responseStatus = false;

/**
 * Режим работы ИБП.
 * P - Power on mode
 * S - Режим ожидания
 * Y - Байпас
 * L - Питание от сети
 * B - Питание от АКБ
 * T - Тест батарей
 * F - Ошибка
 * E - Режим Eco
 * C - Режим Конвертора
 * D - Выключен
 */
String mode;

/**
 * Входное напряжение [В]
 */
String vIn;

/**
 * Входная частота [Гц]
 */
String fIn;

/**
 * Выходное напряжение [В]
 */
String vOut;

/**
 * Выходная частота [Гц]
 */
String fOut;

/**
 * Выходной ток [А]
 */
 String iOut;

 /**
  * Уровень загрузки %
  */
 String load;

/**
 * Положительное напряжение шины DC
 */
 String posDC;

 /**
  * Отрицательное напряжение шины DC
  */
 String negDC;

 /**
  * Напряжение АКБ
  */
 String vBat;

 /**
  * Температура датчика температуры
  */
 String temp;

void loop() {
  responseStatus = true;
  setSPBMode();
  setSPBGenerealParams();

  logfile.print(getTime());
  logfile.print(", ");
  logfile.print(mode);
  logfile.print(", ");
  logfile.print(vIn);
  //setSPBWarnings();
  //setSPBErrors();
  //setSPBBatteriesState();

  //printVars();
  delay(3000);
}

boolean readSPBSerialBuffer() {
  memset(buffer, 0, sizeof buffer);
  
  int i=0;
  if (SPBSerial.available() > 0 && i < SPB_SERIAL_BUFFER_SIZE)
  {
    delay(10);
    while(SPBSerial.available()) 
    {
      buffer[i++] = SPBSerial.read();
    }
    buffer[i]='\0';
  }
  if (i == 0) {
    return false;
  } else {
    return true;
  }
}

String validateSPBSerialBuffer() {
  boolean readStatus = readSPBSerialBuffer();
  if (readStatus == false) {
    responseStatus = false;
  }

  String bufStr = String(buffer); 
  int startIndex = bufStr.indexOf(SPB_RESP_START_CHAR);
  if (startIndex != 0) {
    responseStatus = false;
  }
  
  String res = bufStr.substring(1);
  return res;
}

//void printVars() {
//  Serial.println("==============================================");
////  Serial.print("=   Time: "); Serial.println(getTime());
//  Serial.print("=   Work mode: "); Serial.println(mode);
//  Serial.print("=   V_in: "); Serial.println(vIn);
//  Serial.print("=   F_in: "); Serial.println(fIn);
//  Serial.print("=   V_out: "); Serial.println(vOut);
//  Serial.print("=   F_out: "); Serial.println(fOut);
//  Serial.print("=   I_out: "); Serial.println(iOut);
//  Serial.print("=   Load: "); Serial.println(load);
//  Serial.print("=   Positive DC: "); Serial.println(posDC);
//  Serial.print("=   Negative DC: "); Serial.println(negDC);
//  Serial.print("=   V_bat: "); Serial.println(vBat);
//  Serial.print("=   Temp: "); Serial.println(temp);
//  Serial.println("==============================================");
//  Serial.println();
//}


/**
 * Считывание режима работы SPB
 */
void setSPBMode() {
  SPBSerial.print("QMOD \r");
  delay(50);

  mode = validateSPBSerialBuffer();
  if (mode.length() != 2) {
    responseStatus = false;
    return;
  }
}

/**
 * Считывание общих парамеров SPB (Напряжение, частота, ..)
 */
void setSPBGenerealParams() {
  SPBSerial.print("QGS \r");
  delay(200);

  String spbResponse = validateSPBSerialBuffer();
  if (spbResponse.length() != 62) {
      responseStatus = false;
      return;
  }

  vIn  = spbResponse.substring(0, 5);
  fIn  = spbResponse.substring(6, 10);
  vOut = spbResponse.substring(11, 16);
  fOut = spbResponse.substring(17, 21);
  iOut = spbResponse.substring(22, 27);
  load = spbResponse.substring(28, 31);
  posDC = spbResponse.substring(32, 37);
  negDC = spbResponse.substring(38, 43);
  vBat = spbResponse.substring(44, 49);
  temp = spbResponse.substring(56, 61); 
}

/**
 * Считывание предупреждений SPB
 */
//void setSPBWarnings() {
//  SPBSerial.print("QWS \r");
//  delay(200);
//
//  String spbResponse = validateSPBSerialBuffer();
//  if (spbResponse.length() != 62) {
//      responseStatus = false;
//      return;
//  }
//}

//Установка ошибки SPB
//void setSPBErrors() {
//  SPBSerial.print("QFS \r");
//  delay(200);
//  readSPBSerialBuffer();
//  //printBuffer();
//}

//Установка состояния батарей
//void setSPBBatteriesState() {
//  SPBSerial.print("QBV \r");
//  delay(100);
//  readSPBSerialBuffer();
//  //printBuffer();
//}

String getTime() {
  DateTime now = RTC.now();
  String res = String(now.day(), DEC);
  res += "/";
  res += String(now.month(), DEC);
  res += "/";
  res += String(now.year(), DEC);
  res += ' ';
  res += String(now.hour(), DEC);
  res += ':';
  res += String(now.minute(), DEC);
  res += ':';
  res += String(now.second(), DEC);
  
  return res;
}


