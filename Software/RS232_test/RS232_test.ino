#include <SoftwareSerial.h>

SoftwareSerial SPBSerial(A3, A2); //Serial 

String inputString = "";
                      
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  String inputString = "";
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.println("Hello SPB-30!");

  // set the data rate for the SoftwareSerial port
  SPBSerial.begin(2400);
}

void loop() { // run over and over
  Serial.println("Send QPI");
  SPBSerial.print("QGS \r");
  delay(300);
  
  SPBserialEvent();
  Serial.println(inputString);
  inputString = "";

  delay(1000);
}

void SPBserialEvent() {
  while (SPBSerial.available()) {
    char inChar = (char)SPBSerial.read();
    inputString += inChar;
  }
}
