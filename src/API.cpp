/*
 * API.cpp
 *
 *  Created on: Jan 29, 2020
 *      Author: György Kovács
 */
#include "Ch376msc.h"


/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::dirInfoRead(){
	uint8_t tmpReturn;
	if(_interface == UARTT){
		sendCommand(CMD_DIR_INFO_READ);// max 16 files 0x00 - 0x0f
		write(0xff);// current file is 0xff
		tmpReturn = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_DIR_INFO_READ);// max 16 files 0x00 - 0x0f
		write(0xff);// current file is 0xff
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	rdFatInfo();
	return tmpReturn;
}
/////////////////////////////////////////////////////////////////
void Ch376msc::writeFatData(){// see fat info table under next filename
	uint8_t fatInfBuffer[32]; //temporary buffer for raw file FAT info
	memcpy ( &fatInfBuffer, &_fileData,  sizeof(fatInfBuffer) ); //copy raw data to temporary buffer
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_WR_OFS_DATA);
	write((uint8_t)0x00);
	write(32);
	for(uint8_t d = 0;d < 32; d++){
		write(fatInfBuffer[d]);
	}
	if(_interface == SPII) spiEndTransfer();
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::mount(){ // return ANSWSUCCESS or ANSW DISK DISCON
	uint8_t tmpReturn = 0;
	if(_interface == UARTT) {
		sendCommand(CMD_DISK_MOUNT);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_DISK_MOUNT);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}//end if interface
	if(!_errorCode && tmpReturn != ANSW_USB_INT_SUCCESS){
		setError(tmpReturn);
	}
	return tmpReturn;
}
///////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileEnumGo(){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT){
		sendCommand(CMD_FILE_ENUM_GO);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_ENUM_GO);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	if(!_errorCode && (tmpReturn != ANSW_USB_INT_DISK_READ) && (tmpReturn != ANSW_ERR_MISS_FILE)){
		setError(tmpReturn);
	}
	return tmpReturn;
}
//////////////////////////////////////////////////////////////
uint8_t Ch376msc::byteRdGo(){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT) {
		sendCommand(CMD_BYTE_RD_GO);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_BYTE_RD_GO);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	if(!_errorCode && (tmpReturn != ANSW_USB_INT_DISK_READ) && (tmpReturn != ANSW_USB_INT_SUCCESS)){
		setError(tmpReturn);
	}
	return tmpReturn;
}

//////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileCreate(){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT) {
		sendCommand(CMD_FILE_CREATE);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_CREATE);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	return tmpReturn;
}
void Ch376msc::rdFatInfo(){
	uint8_t fatInfBuffer[32]; //temporary buffer for raw file FAT info
	uint8_t dataLength;
	bool owrflow = false;
	if(_interface == UARTT){
		sendCommand(CMD_RD_USB_DATA0);
		dataLength = readSerDataUSB();
		if(dataLength > sizeof(fatInfBuffer)){
			owrflow = true;
			dataLength = sizeof(fatInfBuffer);
		}
		for(uint8_t s =0;s < dataLength;s++){
			fatInfBuffer[s] = readSerDataUSB();// fill up temporary buffer
		}//end for
	} else {
		spiBeginTransfer();
		sendCommand(CMD_RD_USB_DATA0);
		dataLength = spiReadData();
		if(dataLength > sizeof(fatInfBuffer)){
			owrflow = true;
			dataLength = sizeof(fatInfBuffer);
		}
		for(uint8_t s =0;s < dataLength;s++){
			fatInfBuffer[s] = spiReadData();// fill up temporary buffer
		}//end for
		spiEndTransfer();
	}
	if(owrflow){
		setError(ERR_OVERFLOW);
	} else {
		memcpy ( &_fileData, &fatInfBuffer, sizeof(fatInfBuffer) ); //copy raw data to structured variable
	}
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::byteWrGo(){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT) {
		sendCommand(CMD_BYTE_WR_GO);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_BYTE_WR_GO);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	if(!_errorCode && (tmpReturn != ANSW_USB_INT_DISK_WRITE) && (tmpReturn != ANSW_USB_INT_SUCCESS)){
		setError(tmpReturn);
	}
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::reqByteRead(uint8_t a){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT){
		sendCommand(CMD_BYTE_READ);
		write(a); // request data stream length for reading, 00 - FF
		write((uint8_t)0x00);
		tmpReturn= readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_BYTE_READ);
		write(a); // request data stream length for reading, 00 - FF
		write((uint8_t)0x00);
		spiEndTransfer();
		tmpReturn= spiWaitInterrupt();
	}
	if(!_errorCode && (tmpReturn != ANSW_USB_INT_SUCCESS) && (tmpReturn != ANSW_USB_INT_DISK_READ)){
		setError(tmpReturn);
	}
	return tmpReturn;
}

////////////////////////////////////////////////////////////////
uint8_t Ch376msc::reqByteWrite(uint8_t a){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT) {
		sendCommand(CMD_BYTE_WRITE);
		write(a); // request data stream length for writing, 00 - FF
		write((uint8_t)0x00);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_BYTE_WRITE);
		write(a); // request data stream length for writing, 00 - FF
		write((uint8_t)0x00);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	if(!_errorCode && (tmpReturn != ANSW_USB_INT_SUCCESS) && (tmpReturn != ANSW_USB_INT_DISK_WRITE)){
		setError(tmpReturn);
	}
	return tmpReturn;
}
/////////////////////////////////////////////////////////////////
void Ch376msc::sendFilename(){
	uint8_t temp;
	if(_chipVer < MIN_CHIP_FW && !_dirDepth){ // old fw and in root dir
		if(_interface == SPII) spiBeginTransfer();
		sendCommand(CMD_SET_FILE_NAME);
		write((uint8_t)0x00);
		if(_interface == SPII) spiEndTransfer();
		temp = openFile();
		if(temp == ANSW_USB_INT_SUCCESS){
			temp = readVar8(0xCF);
			if(temp){
				writeVar32(0x4C, readVar32(0x4C) + ((uint16_t)temp << 8));
				writeVar32(0x50, readVar32(0x50) + ((uint16_t)temp << 8));
				writeVar32(0x70, 0);
			}
		}
	}
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_SET_FILE_NAME);
	if(!_dirDepth) write(0x2f); // "/" root directory
	print(_filename); // filename
	//write(0x5C);	// ez a "\" jel
	write((uint8_t)0x00);	// terminating null character
	if(_interface == SPII) spiEndTransfer();
}
/////////////////////////////////////////////////////////////////
void Ch376msc::rdDiskInfo(){
	uint8_t dataLength;
	uint8_t tmpReturn;
	uint8_t tmpdata[9];
	if(_chipVer < MIN_CHIP_FW){
		if(readVar8(CMD_VAR_DISK_STATUS) >= DEF_DISK_READY) writeVar8(CMD_VAR_DISK_STATUS, DEF_DISK_MOUNTED);
	}
	if(_interface == UARTT){
		sendCommand(CMD_DISK_QUERY);
		tmpReturn= readSerDataUSB();
		if(tmpReturn == ANSW_USB_INT_SUCCESS){
			sendCommand(CMD_RD_USB_DATA0);
			dataLength = readSerDataUSB();
			for(uint8_t s =0;s < dataLength;s++){
				tmpdata[s] = readSerDataUSB();// fill up temporary buffer
			}//end for
		}//end if success
	} else {
		spiBeginTransfer();
		sendCommand(CMD_DISK_QUERY);
		spiEndTransfer();
		tmpReturn= spiWaitInterrupt();
		if(tmpReturn == ANSW_USB_INT_SUCCESS){
			spiBeginTransfer();
			sendCommand(CMD_RD_USB_DATA0);
			dataLength = spiReadData();
			for(uint8_t s =0;s < dataLength;s++){
				tmpdata[s] = spiReadData();// fill up temporary buffer
			}//end for
			spiEndTransfer();
		}//end if success
	}//end if UART
	if(tmpReturn != ANSW_USB_INT_SUCCESS){// unknown partition issue #22
		if(!_errorCode){
			setError(tmpReturn);
		}//end if error
	} else {
		clearError();
		_deviceAttached = true;
		memcpy ( &_diskData, &tmpdata, sizeof(tmpdata) ); //copy raw data to structured variable
	}
}
///////////////////////////////////////////////////////////////
uint8_t Ch376msc::dirCreate(){
	uint8_t tmpReturn;
	if(_interface == UARTT) {
		sendCommand(CMD_DIR_CREATE);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_DIR_CREATE);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
void Ch376msc::driveAttach(){
		uint8_t tmpReturn = 0;
		if(_driveSource == 0){//if USB
			setMode(MODE_HOST_1);//TODO:if 5F failure
			setMode(MODE_HOST_2);
			if(_interface == UARTT){
				tmpReturn = readSerDataUSB();
			} else {
				tmpReturn = spiWaitInterrupt();
			}//end if interface
		}//end if usb
			if(tmpReturn == ANSW_USB_INT_CONNECT){// TODO: itt figyelni
				for(uint8_t a = 0;a < 5;a++){//try to mount, delay in worst case ~(number of attempts * ANSWTIMEOUT ms)
					tmpReturn = mount();
					if(tmpReturn == ANSW_USB_INT_SUCCESS){
						clearError();
						_deviceAttached = true;
						break;
					} else if(_errorCode != ERR_TIMEOUT){
						break;
					}//end if Success
				}//end for
			} else driveDetach();
		if(_deviceAttached)	rdDiskInfo();
}
///////////////
void Ch376msc::driveDetach(){
	if(_driveSource == 0){//if USB
		setMode(MODE_HOST_0);
	}
	_deviceAttached = false;
	rstDriveContainer();
	rstFileContainer();
}

void Ch376msc::queryChipVersion(){
	if(_interface == UARTT){
		sendCommand(CMD_GET_IC_VER);
		//write(mode);
		_chipVer = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_GET_IC_VER);
		//write(mode);
		delayMicroseconds(10);
		_chipVer = spiReadData();
		spiEndTransfer();
		//delayMicroseconds(40);
	}
}

uint8_t Ch376msc::readVar8(uint8_t addrs){
	uint8_t temp;
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_READ_VAR8);
	write(addrs);
	if(_interface == SPII){
		temp = spiReadData();
		spiEndTransfer();
	} else {
		//UART
		temp = readSerDataUSB();
	}
	return temp;
}

void Ch376msc::writeVar8(uint8_t addrs, uint8_t data){   /* Write 8-bit variable inside CH376 chip*/
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_WRITE_VAR8);
	write(addrs);
	write(data);
	if(_interface == SPII) spiEndTransfer();
}

uint32_t Ch376msc::readVar32(uint8_t addrs){
	fSizeContainer temp;
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_READ_VAR32);
	write(addrs);
	if(_interface == SPII){
		temp.b[0] = spiReadData();
		temp.b[1] = spiReadData();
		temp.b[2] = spiReadData();
		temp.b[3] = spiReadData();
		spiEndTransfer();
	} else {
		//UART
		temp.b[0] = readSerDataUSB();
		temp.b[1] = readSerDataUSB();
		temp.b[2] = readSerDataUSB();
		temp.b[3] = readSerDataUSB();
	}

	return temp.value;
}

void Ch376msc::writeVar32(uint8_t addrs, uint32_t data){
	fSizeContainer temp;
	temp.value = data;
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_WRITE_VAR32);
	write(addrs);
	write(temp.b[0]);
	write(temp.b[1]);
	write(temp.b[2]);
	write(temp.b[3]);
	if(_interface == SPII) spiEndTransfer();
}
