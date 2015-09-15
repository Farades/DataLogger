//*****************************************************************************************//
//                            SD Card Information and Reading Tests
//                                Adrian Jones, February 2015
//     Fixing the capacity calculation error of "CardInfo" example sketch in SD library
//*****************************************************************************************//
 
// Build 1
//   r1 150223 - initial build 
//*****************************************************************************************//
#define build 1
#define revision 1
//*****************************************************************************************//

//  * SD card attached to SPI bus as follows:
// ** MOSI D11, MISO D12, CLK D13, CS D10

#include <SPI.h>
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

#define chipSelect 10            // chip select pin

//*****************************************************************************************//void setup()
//                                      Initial Setup{
//*****************************************************************************************//  // Open serial communications and wait for port to open:
void setup() {
  Serial.begin(57600); 
  Serial.println(F("*************************************************************************"));
  Serial.println(F("Title:    SD Card Info and Reading Tests"));
  Serial.print(F("File:     ")); Serial.println(__FILE__); 
  Serial.print(F("Build:    ")); Serial.print(build);     Serial.print(F(".")); Serial.println(revision);
  Serial.print(F("Compile:  ")); Serial.print(__TIME__);  Serial.print(F(", "));   Serial.println(__DATE__);
  Serial.print(F("Free RAM: ")); Serial.print(freeRam()); Serial.println(F("B\r\n"));
  Serial.println(F("*************************************************************************"));

  Serial.print(F("\nInitializing SD card..."));
  pinMode(10, OUTPUT);     // change this to 53 on a mega

  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println(F("\t- failed."));
    return;
  } else {
    Serial.println(F("\t- pass."));
  }

   Serial.print(F("Card type is ")); // print the type of card
   switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println(F("SD1"));
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println(F("SD2"));
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println(F("SDHC"));
      break;
    default:
      Serial.println(F("Unknown"));
  }

   if (!volume.init(card)) {     // try to open the 'volume'/'partition' - should be FAT16 or FAT32
     Serial.println(F("Could not find FAT16/FAT32 partition."));
     return;
  }

  // print the type and size of the first FAT-type volume
  unsigned long volumesize;
  Serial.print("Volume type is FAT");
  Serial.println(volume.fatType(), DEC);

  Serial.println(F("Volume Size: "));              // print the size information
  Serial.println(F("\t- Block Size: 512 Bytes"));  // SD card blocks are always 512 bytes
  Serial.print(F("\t- Blocks Per Cluster: "));
  Serial.println(volume.blocksPerCluster());    // clusters are collections of blocks
  Serial.print(F("\t- Cluster Count: "));
  Serial.println(volume.clusterCount());
  Serial.print(F("\t- Total Size: "));
  volumesize = volume.blocksPerCluster() * volume.clusterCount() / 2;                            
  Serial.print(volumesize);
  Serial.print(F("KB,  "));
  float gb = (float) volumesize/1024.0;
  Serial.print(gb, 2);
  Serial.print(F("MB,  "));
  gb = gb /1024.0;
  Serial.print(gb, 2);
  Serial.println(F("GB"));
  Serial.println(F("\n*************************************************************************"));

  Serial.println(F("\nFiles found on the card (name, date and size in bytes): "));
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
}


//*****************************************************************************************//
//                                      MAIN LOOP
//*****************************************************************************************//
void loop() {
  
}

// ********************************************************************************** //
//                              OPERATION ROUTINES
// ********************************************************************************** //
// FREERAM: Returns the number of bytes currently free in RAM  
int freeRam(void) {
  extern int  __bss_end, *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  } 
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
}
