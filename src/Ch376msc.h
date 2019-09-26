/*
 * Ch376msc.h
 *
 *  Created on: Feb 25, 2019
 *      Author: György Kovács
 *  Copyright (c) 2019, György Kovács
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 ******************************************************
 * Versions:                                          *
 * ****************************************************
 * v1.4.0 Sep 26, 2019 
 * 	- new functions
 *   	getTotalSectors() - returns a unsigned long number, total sectors on the drive
 *   	getFreeSectors() - returns a unsigned long number, free sectors on the drive
 *   	getFileSystem() - returns a byte number, 0x01-FAT12, 0x02-FAT16, 0x03-FAT32
 * 	- updated example files with a new functions
 * 	- new example file, seraching the oldest/newest file on the flash drive
 * **************************************************** 
 * 	v1.3 Sep 17, 2019
 * 		-bug fix for moveCursor issue #3  
 *	https://github.com/djuseeq/Ch376msc/issues/3
 * ****************************************************
 *  v1.2 Apr 24, 2019
 *  	-bug fix for timing issue on higher SPI clock
 *  	 datasheet 7.3 Time Sequence table (TSC)
 ******************************************************
 *  v1.2 Apr 20, 2019
 *  	-extended with SPI communication
 ******************************************************
 *	v1.1 Feb 25, 2019
 *		-initial version with UART communication
 ******************************************************
 */

#ifndef Ch376msc_H_
#define Ch376msc_H_

#include <Arduino.h>
#include "CommDef.h"
#include <Stream.h>
#include <SPI.h>


#define TIMEOUT 2000 // waiting for data from CH
#define SPICLKRATE 125000 //Clock rate 125kHz				SystemClk  DIV2  MAX
						// max 8000000 (8MHz)on UNO, Mega (16 000 000 / 2 = 8 000 000)


class Ch376msc {
public:
	/////////////Constructors////////////////////////
	Ch376msc(HardwareSerial &usb, uint32_t speed);//HW serial
	Ch376msc(Stream &sUsb);// SW serial
	Ch376msc(uint8_t spiSelect, uint8_t busy);//SPI with MISO as Interrupt pin
	Ch376msc(uint8_t spiSelect, uint8_t busy, uint8_t intPin);
	virtual ~Ch376msc();//destructor
	////////////////////////////////////////////////
	void init();

	uint8_t mount();
	uint8_t dirInfoSave();
	uint8_t openFile();
	uint8_t closeFile();
	uint8_t moveCursor(uint32_t position);
	uint8_t deleteFile();
	uint8_t pingDevice();
	uint8_t listDir();
	uint8_t readFile(char* buffer, uint8_t b_num);
	uint8_t writeFile(char* buffer, uint8_t b_num);
	bool checkDrive(); // check is it any interrupt message came from CH
	//bool listDir(char (*nameBuff)[12],uint32_t *sizeBuff,uint8_t listElements); //376_7
	//void reset();

//set/get

	//uint32_t getComSpeed();
	uint32_t getFreeSectors();
	uint32_t getTotalSectors();
	uint32_t getFileSize();
	uint16_t getYear();
	uint16_t getMonth();
	uint16_t getDay();
	uint16_t getHour();
	uint16_t getMinute();
	uint16_t getSecond();
	uint8_t getStatus();
	uint8_t getFileSystem();
	char* getFileName();
	char* getFileSizeStr();
	bool getDeviceStatus(); // usb device mounted, unmounted
	bool getCHpresence();

	void setFileName(const char* filename);
	void setYear(uint16_t year);
	void setMonth(uint16_t month);
	void setDay(uint16_t day);
	void setHour(uint16_t hour);
	void setMinute(uint16_t minute);
	void setSecond(uint16_t second);

private:
	//
	//uint8_t read();
	void write(uint8_t data);
	void print(const char str[]);
	void spiReady();
	void spiBeginTransfer();
	void spiEndTransfer();
	uint8_t spiWaitInterrupt();
	uint8_t spiReadData();

	uint8_t getInterrupt();
	uint8_t fileEnumGo();
	uint8_t byteRdGo();
	uint8_t fileCreate();
	uint8_t byteWrGo();
	uint8_t reqByteRead(uint8_t a);
	uint8_t reqByteWrite(uint8_t a);
	uint8_t readSerDataUSB();
	uint8_t writeDataFromBuff(char* buffer);
	uint8_t readDataToBuff(char* buffer);
	uint8_t dirInfoRead();
	uint8_t setMode(uint8_t mode);

	void rdFatInfo();
	void setSpeed();
	void sendCommand(uint8_t b_parancs);
	void sendFilename();
	void writeFatData();
	void constructDate(uint16_t value, uint8_t ymd);
	void constructTime(uint16_t value, uint8_t hms);
	void rdDiskInfo();

	///////Global Variables///////////////////////////////
	bool _fileWrite = false; // read or write mode, needed for close operation
	bool _deviceAttached = false;	//false USB levalsztva, true csatlakoztatva
	bool _controllerReady = false; // ha sikeres a kommunikacio
	bool _hwSerial;

	uint8_t _byteCounter = 0; //vital variable for proper reading,writing
	uint8_t _answer = 0;	//a CH jelenlegi statusza INTERRUPT
	uint8_t _spiChipSelect; // chip select pin SPI
	uint8_t _spiBusy; //   busy pin SPI
	uint8_t _intPin; // interrupt pin
	uint16_t _sectorCounter = 0;// variable for proper reading
	uint32_t _speed; // Serial communication speed
	char _filename[12];

	HardwareSerial* _comPortHW; // Serial interface
	Stream* _comPort;

	commInterface _interface;
	fileProcessENUM fileProcesSTM = REQUEST;

	fatFileInfo _fileData;
	diskInfo _diskData;


};//end class

#endif /* Ch376msc_H_ */


