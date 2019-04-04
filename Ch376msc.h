/*
 * Ch376msc.h
 *
 *  Created on: Feb 25, 2019
 *      Author: György Kovács
 */

#ifndef CH376MSC_H_
#define CH376MSC_H_

#include <Arduino.h>
#include "CommDef.h"
#include <Stream.h>



#define TIMEOUT 2000 // waiting for data from CH
#define FATFNAMELENGHT 12 // 11+NULL



class Ch376msc {
public:
	/////////////Constructor////////////////////////
	Ch376msc(HardwareSerial &usb, uint32_t speed);//HW serial
	Ch376msc(Stream &sUsb);// SW serial
	virtual ~Ch376msc();//destructor
	////////////////////////////////////////////////
	void init();

	uint8_t mount();
	uint8_t dirInfoSave();
	uint8_t open();
	uint8_t fileOpen();
	uint8_t fileClose();
	uint8_t moveCursor(uint32_t position);
	uint8_t deleteFile();
	uint8_t pingDevice();
	uint8_t listDir();
	uint8_t readFile(char* buffer, uint8_t b_num);
	uint8_t writeFile(char* buffer, uint8_t b_num);
	bool checkCH(); // check is it any interrupt message came from CH
	//bool listDir(char (*nameBuff)[12],uint32_t *sizeBuff,uint8_t listElements); //376_7
	void reset();

//set/get

	uint32_t getComSpeed();
	uint32_t getFileSize();
	uint16_t getYear();
	uint16_t getMonth();
	uint16_t getDay();
	uint16_t getHour();
	uint16_t getMinute();
	uint16_t getSecond();
	uint8_t getStatus();
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
	uint8_t fileEnumGo();
	uint8_t byteRdGo();
	uint8_t fileCreate();
	uint8_t byteWrGo();
	uint8_t reqByteRead(uint8_t a);
	uint8_t reqByteWrite(uint8_t a);
	uint8_t adatUsbTol();
	uint8_t writeDataFromBuff(char* buffer);
	uint8_t readDataToBuff(char* buffer);
	uint8_t dirInfoRead();
	uint8_t setMode(uint8_t mode);

	void rdUsbData();
	void setSpeed();
	void sendCommand(uint8_t b_parancs);
	void sendFilename();
	void writeFatData();
	void constructDate(uint16_t value, uint8_t ymd);
	void constructTime(uint16_t value, uint8_t hms);
//#define TEST
#ifdef TEST
	void fileReOpen(); // if cluster is full
	void wrDummyByte(uint8_t asd);
#endif
	///////Variables///////////////////////////////
	bool _fileWrite; // read or write mode, needed for close operation
	bool _mediaStatus;				//false USB levalsztva, true csatlakoztatva
	bool _controllerReady; // ha sikeres a kommunikacio

	uint8_t _byteCounter; //vital variable for proper reading,writing
	uint8_t _answer;	//a CH jelenlegi statusza INTERRUPT
	uint8_t _tmpReturn;// variable to hold temporary data
	uint16_t _sectorCounter;// variable for proper reading
	uint32_t _ul_oldMillis;			// e.g. timeout

	HardwareSerial* _comPortHW; // Serial interface
	Stream* _comPort;
	uint32_t _speed; // Serial communication speed
	char _filename[FATFNAMELENGHT];
	/////////////////////////////////////////////////
	fileProcessENUM fileProcesSTM;
	fatFileInfo _fileData;


};//end class

#endif /* CH376MSC_H_ */


