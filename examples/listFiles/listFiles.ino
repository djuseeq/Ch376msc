#include <Ch376msc.h>

/*
 * tested on Arduino Mega, leave the default jumper settings for the baud rate (9600) on the CH376, 
 * the library will set it up the chosen speed(HW serial only)
 */
Ch376msc flashDrive(Serial1, 115200); // hardware serial , speed(e.g. 9600, 19200, 57600, 115200)


void setup() {
  Serial.begin(115200);
  flashDrive.init();

  //ListFiles
  while(flashDrive.listDir()){ // reading next file
    Serial.print(flashDrive.getFileName()); // get the actual file name
    Serial.print(" : ");
    Serial.print(flashDrive.getFileSize()); // get the actual file size in bytes
    Serial.print(" >>>\t");
    Serial.println(flashDrive.getFileSizeStr()); // get the actual file size in formatted string
  }

}

void loop() {
  

}
