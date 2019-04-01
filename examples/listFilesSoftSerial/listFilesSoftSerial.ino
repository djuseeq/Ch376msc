#include <Ch376msc.h>
#include <SoftwareSerial.h>

//Important! First create a soft serial object, after create a Ch376 object
SoftwareSerial mySerial(7, 6); // RX, TX

Ch376msc flashDrive(mySerial); // Ch376 object with software serial

 // buffer for reading / writing
//char adatBuffer[255];// max length 255, 254 char + 1 NULL character


void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);// Important! First initialize soft serial object and after Ch376
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
