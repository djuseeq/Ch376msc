# Arduino library for CH376 file manage control chip
Supports read / write files to USB flash drive or SD card. The chip supports the FAT12, FAT16 and FAT32 file systems, meaning the chip does the hard work, the MCU doesn't have to deal with the FAT file system

## Getting Started
Configure the jumpers on the module depending on which communication protocol you are using(see API reference)

![Alt text](extras/JumperSelect.png?raw=true "Setting")

## Versioning
v1.4.2*test* 2020
 > - support SD card manage(API ref. - setSource(),if the SD card socket is not available on the module,
 > then modification on the module is required, please read Getting Started section)
  
v1.4.1 Dec 22, 2019 
  - supports more architectures(see Tested boards table below) - issue #11
  - constructor update (BUSY pin is not longer used)
  - improved logic to the mount/unmount flash drive
  - directory support ( cd(); function )
  - use advanced file listing with (*) wildcard character(API reference, listDir() function)

v1.4.0 Sep 26, 2019 
  - new functions
     - getTotalSectors() - returns a unsigned long number, total sectors on the drive
     - getFreeSectors() - returns a unsigned long number, free sectors on the drive
     - getFileSystem() - returns a byte number, 0x01-FAT12, 0x02-FAT16, 0x03-FAT32
  - updated example files with a new functions
  - new example file, searching for the oldest/newest file on the flash drive

v1.3.1 Sep 20, 2019 
  - rearrange the folder structure to be 1.5 library format compatible

v1.3 Sep 17, 2019 
  - bug fix for moveCursor issue #3 , minor changes

v1.2.1 Apr 24, 2019 
  - In use of SPI, CS pin on the module must to be pulled to VCC otherwise communication can be instable on a higher clock rate
  - bug fix for timing issue on a higher clock rate (TSC)
                  
v1.2 Apr 20, 2019 
  - extended with SPI communication

v1.1 Feb 25, 2019 
  - initial version with UART communication

## API Reference
```C++
//The SPI communication speed is reduced to 125 kHz because of stability if long cables or breadboard is used. 
//Possible speed options(Hz): 125000,250000,500000,1000000,2000000,4000000 
//To change, edit /src/Ch376msc.h file. Find the #define SPICLKRATE line and change the value.
    //CONSTRUCTORS
     //UART
      //For hardware serial leave the communication settings on the module at default speed (9600bps) 
       Ch376msc(HardwareSerial, speed);//Select the serial port to which the module is connected and the desired speed(9600, 19200, 57600, 115200)
    
      //For software serial select the desired communication speed on the module(look on the picture above)
       Ch376msc(SoftwareSerial);
       
     //SPI
      //If no other device is connected to the SPI port it`s possible to save one MCU pin
       Ch376msc(spiSelect);// ! Don`t use this if the SPI port is shared with other devices
       
      //If the SPI port is shared with other devices, use this constructor and one extra MCU pin need to be sacrificed for the INT pin
       Ch376msc(spiSelect, interruptPin);
////////////////////
    
	 // Must be initialized before any other command are called from this class.
	init();
	
	 // call frequently to get any interrupt message of the module(attach/detach drive)
	checkDrive(); //return TRUE if an interrupt request has been received, FALSE if not.
	
	 // can call before any file operation
	driveReady(); //returns FALSE if no drive is present or TRUE if drive is attached and ready.
	
	 // check the communication between MCU and the CH376
	pingDevice(); //returns FALSE if there is a communication failure, TRUE if communication  is ok
	
	 // 8.3 filename, also called a short filename is accepted 
	setFileName(filename);//8 char long name + 3 char long extension
	
	 // open file before any file operation. Use first setFileName() function
	openFile();
	
	 // always call this after finishing with file operations otherwise data loss or file corruption may occur
	closeFile();
	
	 // repeatedly call this function to read data to buffer until the return value is TRUE
	readFile(buffer, length);// buffer - char array, buffer size`
	
	 // repeatedly call this function to write data to the drive until there is no more data for write or the return value is FALSE
	writeFile(buffer, length);// buffer - char array, string size in the buffer
	
	 // switch between source drive's, 0 = USB(default), 1 = SD card
	 // !!Before calling this function and activate the SD card please do the required modification 
	 // on the pcb, please read Getting Started section otherwise you can damage the CH376 chip.
	setSource(srcDrive);// 0 or 1
	
	setYear(year); // 1980 - 2099
	setMonth(month);// 1 - 12
	setDay(day);// 1 - 31
	setHour(hour);// 0 - 23
	setMinute(minute);// 0 - 59
	setSecond(second);// 0 - 59 saved with 2 second resolution (0, 2, 4 ... 58)
	
	 // when new file is created the defult file creation date/time is (2004-1-1 0.0.0), 
	 // it is possible to change date/time with this function, use first set functions above to set the file attributes
	saveFileAttrb();
	
	 // move the file cursor to specified position
	moveCursor(position);// 00000000h - FFFFFFFFh
	
	 // delete the specified file, use first setFileName() function
	deleteFile();
	
	 // repeatedly call this function with getFileName until the return value is TRUE to get the file names from the current directory
	 // limited possibility to use with wildcard character e.g. listDir("AB*") will list files with names starting with AB
	 // listDir("*AB") will not work, wildcard char+string must to be less than 8 character long
	 // if no argument is passed while calling listDir(), all files will be printed from the current directory
	listDir();// returns FALSE if no more file is in the current directory
	 
	 //dirPath = e.g. "/DIR1/DIR2/DIR3" , "/" - root dir
	 //CreateDir = 0(open directories if they not exist, don`t create them) or 1(create directories if they do not exist and open them)
	 //if working in subfolders, before file operations ALWAYS call this function with the full directory path
	 //limited to 3 subfolders depth (see /src/Ch376msc.h file. MAXDIRDEPTH) and 8 character long directory names
	cd(dirPath,CreateDir);// returns byte value,see example .ino
	
	getFreeSectors();// returns unsigned long value
	getTotalSectors();// returns unsigned long value
	getFileSize();// returns unsigned long value (byte)
	getYear();// returns int value
	getMonth();// returns int value
	getDay();// returns int value
	getHour();// returns int value
	getMinute();// returns int value
	getSecond();// returns int value
	
	getFileSystem();// returns byte value, 01h-FAT12, 02h-FAT16, 03h-FAT32
	getFileName();// returns the file name in a 11+1 character long string value
	getFileSizeStr();// returns file size in a formatted 9+1 character long string value
	getFileAttrb();// returns byte value, see /src/CommDef.h , (File attributes)
```
## Tested boards
|Board(arch) | SPI | HW Serial | SW Serial|
|:---|:---:|:---:|:---:|
|Arduino (AVR)|OK|OK|OK|
|DUE (SAM)|OK(with INT pin)|OK|NO|
|ZERO (SAMD)|?compile ok|?|NO|
|*STM32 cores|OK|!NO|NO|
|**STM32duino|OK|OK|NO|
|***ESP8266|OK(with INT pin)|NO|OK|

Be careful when choosing SoftSerial because it has its own limitations. See [issues#15](https://github.com/djuseeq/Ch376msc/issues/15)

> `*` Tested on NUCLEO F446RE(no signal at all on UART ports)

> `**` Tested on Generic STM32F103C alias Blue pill with STM32duino bootloader

> `***` Tested on NodeMCU,(i'm not familiar with ESP MCUs) it looks they have default enabled WDT so i have to call
>	`yield()` periodically during file operations, otherwise ESP will restart with a ugly message.
>	Working SPI configuration (for me)is MISO-12(D6), MOSI-13(D7), SCK-14(D5), CS-4(D2), INT-5(D1)

### Acknowledgments

Thanks for the idea to [Scott C](https://arduinobasics.blogspot.com/2015/05/ch376s-usb-readwrite-module.html)

## License
The MIT License (MIT)

Copyright (c) 2019 György Kovács

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

