#include <SoftwareSerial.h>

SoftwareSerial SPBSerial(A3, A2); //Serial 

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.println("Hello SPB-30!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(2400);
}

void loop() { // run over and over
  Serial.println("Send QPI");
  mySerial.print("QGS \r");
  delay(300);
  
  int i=0;
  char buffer[1000];
  if (mySerial.available() > 0)
  {
    delay(30);
    while(mySerial.available()) 
    {
      buffer[i++] = mySerial.read();
    }
    buffer[i++]='\0';
  }

  if (i > 0) {
    Serial.println(String(buffer));
  }

  delay(1000);
}
