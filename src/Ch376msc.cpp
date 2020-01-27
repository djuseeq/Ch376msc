/*
 * Ch376msc.cpp
 *
 *  Created on: Feb 25, 2019
 *      Author: György Kovács
 */

#include "Ch376msc.h"

//with HW serial
Ch376msc::Ch376msc(HardwareSerial &usb, uint32_t speed) { // @suppress("Class members should be properly initialized")
	_interface = UARTT;
	_comPortHW = &usb;
	_comPort = &usb;
	_speed = speed;
	_hwSerial = true;
}
//with soft serial
Ch376msc::Ch376msc(Stream &sUsb) { // @suppress("Class members should be properly initialized")
	_interface = UARTT;
	_comPort = &sUsb;
	_speed = 9600;
	_hwSerial = false;
}

Ch376msc::Ch376msc(uint8_t spiSelect, uint8_t intPin){ // @suppress("Class members should be properly initialized")
	_interface = SPII;
	_intPin = intPin;
	_spiChipSelect = spiSelect;
	_speed = 0;
}

//with SPI, MISO as INT pin(the SPI bus is only available for CH376, SPI bus can`t be shared with other SPI devices)
Ch376msc::Ch376msc(uint8_t spiSelect){ // @suppress("Class members should be properly initialized")
	_interface = SPII;
	_intPin = MISO; // use the SPI MISO for interrupt JUST if no other device is using the SPI bus!!
	_spiChipSelect = spiSelect;
	_speed = 0;
}
Ch376msc::~Ch376msc() {
	//  Auto-generated destructor stub
}

/////////////////////////////////////////////////////////////////
void Ch376msc::init(){
	delay(60);//wait for VCC to normalize
	if(_interface == SPII){
		if(_intPin != MISO){
			pinMode(_intPin, INPUT_PULLUP);
		}
		pinMode(_spiChipSelect, OUTPUT);
		digitalWrite(_spiChipSelect, HIGH);
		SPI.begin();
		spiBeginTransfer();
		sendCommand(CMD_RESET_ALL);
		spiEndTransfer();
		delay(100);// wait after reset command
		if(_intPin == MISO){ // if we use MISO as interrupt pin, then tell it to the device ;)
			spiBeginTransfer();
			sendCommand(CMD_SET_SD0_INT);
			write(0x16);
			write(0x90);
			spiEndTransfer();
		}//end if
	} else {//UART
		if(_hwSerial) _comPortHW->begin(9600);// start with default speed
		sendCommand(CMD_RESET_ALL);
		delay(100);// wait after reset command, according to the datasheet 35ms is required, but that was too short
		if(_hwSerial){ // if Hardware serial is initialized
			setSpeed(); // Dynamically configure the com speed
		}
	}//end if UART
	_controllerReady = pingDevice();// check the communication
	if(_controllerReady) _errorCode = 0;// reinit clear last error code
	setMode(MODE_HOST_0);
	checkIntMessage();
}
/////////////////////////////////////////////////////////////////
void Ch376msc::setSpeed(){ //set communication speed for HardwareSerial and device
	if(_speed == 9600){ // default speed for CH
 // do nothing
	} else {
		sendCommand(CMD_SET_BAUDRATE);
		switch (_speed) {
			case 19200:
				_comPortHW->write(uint8_t(0x02));//detach freq. coef
				_comPortHW->write(uint8_t(0xD9));//detach freq. constant
				break;
			case 57600:
				_comPortHW->write(uint8_t(0x03));
				_comPortHW->write(uint8_t(0x98));
				break;
			case 115200:
				_comPortHW->write(uint8_t(0x03));
				_comPortHW->write(uint8_t(0xCC));
				break;
			default:
				_speed = 9600;
				break;
		}//end switch
		_comPortHW->end();
		_comPortHW->begin(_speed);
		delay(2);// according to datasheet 2ms
	}// end if

}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::pingDevice(){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT){
		sendCommand(CMD_CHECK_EXIST);
		write(0x01); // ez ertek negaltjat adja vissza
		if(readSerDataUSB() == 0xFE){
			tmpReturn = 1;//true
		}
	} else {
		spiBeginTransfer();
		sendCommand(CMD_CHECK_EXIST);
		write(0x01); // ez ertek negaltjat adja vissza
		if(spiReadData() == 0xFE){
			tmpReturn = 1;//true
		}
		spiEndTransfer();
	}
	if(!tmpReturn){
		_errorCode = 1;
	}
	return tmpReturn;
}
/////////////////////////////////////////////////////////////////
bool Ch376msc::driveReady(){//returns TRUE if the drive ready
	uint8_t tmpReturn = 0;
	if(_driveSource == 1){//if SD
		if(!_dirDepth){// just check SD card if it's in root dir
			setMode(MODE_DEFAULT);//reinit otherwise is not possible to detect if the SD card is removed
			setMode(MODE_HOST_SD);
			tmpReturn = mount();
			if(tmpReturn == ANSW_USB_INT_SUCCESS){
				if(!_sdMountFirst){// get drive parameters just once
					_sdMountFirst = true;
					rdDiskInfo();
				}//end if _sdMountF
			} else {
				driveDetach(); // do reinit otherwise mount will return always "drive is present"
				_sdMountFirst = false;
				_errorCode = tmpReturn;
			}//end if INT_SUCCESS
		} else tmpReturn = ANSW_USB_INT_SUCCESS;//end if not ROOT
	} else {//if USB
		//if(_interface == UARTT){
		//	sendCommand(CMD_DISK_CONNECT);
		//	tmpReturn = readSerDataUSB();
		//} else {
			//spiBeginTransfer();
			//sendCommand(CMD_DISK_CONNECT);
			//spiEndTransfer();
			//tmpReturn = spiWaitInterrupt();
			tmpReturn = mount();
		//}//end if interface
	}//end if source
	if(tmpReturn == ANSW_USB_INT_SUCCESS){
		_deviceAttached = true;
		return true;
	} else {
		_deviceAttached = false;
		return false;
	}

}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::setMode(uint8_t mode){
	uint8_t tmpReturn = 0;
	if(_interface == UARTT){
		sendCommand(CMD_SET_USB_MODE);
		write(mode);
		tmpReturn = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_SET_USB_MODE);
		write(mode);
		delayMicroseconds(10);
		tmpReturn = spiReadData();
		spiEndTransfer();
		delayMicroseconds(40);
	}
	return tmpReturn; // success or fail
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
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
bool Ch376msc::checkIntMessage(){ //always call this function to get INT# message if thumb drive are attached/detached
	uint8_t tmpReturn = 0;
	bool intRequest = false;
		if(_interface == UARTT){
			while(_comPort->available()){ // while is needed, after connecting media, the ch376 send 3 message(connect, disconnect, connect)
				tmpReturn = readSerDataUSB();
			}//end while
		} else {//spi
			while(!digitalRead(_intPin)){
				tmpReturn = getInterrupt(); // get int message
				delay(10);//sadly but it required for stability, sometime prior attaching drive the CH376 produce more interrupts
			}// end while
		}//end if interface
		switch(tmpReturn){ // 0x15 device attached, 0x16 device disconnect
		case ANSW_USB_INT_CONNECT:
			intRequest = true;
			driveAttach();//device attached
			break;
		case ANSW_USB_INT_DISCONNECT:
			intRequest = true;
			driveDetach();//device detached
			break;
		}//end switch
	return intRequest;
}
/////////////////////////////////////////////////////////////////
void Ch376msc::driveAttach(){
		uint8_t tmpReturn;//, tt;
		if(_driveSource == 0){//if USB
			setMode(MODE_HOST_1);//TODO:if 5F failure
			setMode(MODE_HOST_2);
			if(_interface == UARTT){
				tmpReturn = readSerDataUSB();
			} else {
				tmpReturn = spiWaitInterrupt();
			}//end if interface
			if((tmpReturn == ANSW_USB_INT_CONNECT) || (!tmpReturn)){
				for(uint8_t a = 0;a < 5;a++){
					if(driveReady()){
						_deviceAttached = true;
						tmpReturn = mount();
						if(tmpReturn == ANSW_USB_INT_SUCCESS){
							break;
						} else {
							setError(tmpReturn);
						}//end if mount INT_SUCCESS
					} else { //end if ready
						break;
					}
				}//end for
			} else driveDetach();
		} else {// SD card
			for(uint8_t a = 0;a < 5;a++){
				tmpReturn = mount();
				if(tmpReturn == ANSW_USB_INT_SUCCESS){
					_deviceAttached = true;
					break;
				} else {
					setError(tmpReturn);
				}//end if mount INT_SUCCESS
			}//end for
			if(!_deviceAttached) driveDetach();
		}//end if USB

		if(_deviceAttached)	rdDiskInfo();
}
///////////////
void Ch376msc::driveDetach(){
	_deviceAttached = false;
	if(_driveSource == 0){//if USB
		setMode(MODE_HOST_0);
	}
	memset(&_diskData, 0, sizeof(_diskData));// fill up with NULL disk data container
}
/////////////////Alap parancs kuldes az USB fele/////////////////
void Ch376msc::sendCommand(uint8_t b_parancs){
	if(_interface == UARTT){
	write(0x57);// UART first sync command
	write(0xAB);// UART second sync command
	}//end if
	write(b_parancs);
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::openFile(){
	if(!_deviceAttached) return 0x00;
	if(_interface == UARTT){
		sendCommand(CMD_FILE_OPEN);
		_answer = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_FILE_OPEN);
		spiEndTransfer();
		_answer = spiWaitInterrupt();
	}
	if(_answer == ANSW_USB_INT_SUCCESS){ // get the file size
		dirInfoRead();
	}
	return _answer;
}

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
uint8_t Ch376msc::saveFileAttrb(){
	uint8_t tmpReturn = 0;
	if(!_deviceAttached) return 0x00;
	_fileWrite = 1;
	if(_interface == UARTT) {
		sendCommand(CMD_DIR_INFO_READ);
		write(0xff);// current file is 0xff
		readSerDataUSB();
		writeFatData();//send fat data
		sendCommand(CMD_DIR_INFO_SAVE);
		tmpReturn = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_DIR_INFO_READ);
		write(0xff);// current file is 0xff
		spiEndTransfer();
		spiWaitInterrupt();
		writeFatData();//send fat data
		spiBeginTransfer();
		sendCommand(CMD_DIR_INFO_SAVE);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
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

////////////////////////////////////////////////////////////////
uint8_t Ch376msc::closeFile(){ // 0x00 - w/o filesize update, 0x01 with filesize update
	if(!_deviceAttached) return 0x00;
	uint8_t tmpReturn = 0;
	uint8_t d = 0x00;
	if(_fileWrite == 1){ // if closing file after write procedure
		d = 0x01; // close with 0x01 (to update file length)
	}
	if(_interface == UARTT){
		sendCommand(CMD_FILE_CLOSE);
		write(d);
		tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_CLOSE);
		write(d);
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	memset(&_fileData, 0, sizeof(_fileData));// fill up with NULL file data container
	cd("/", 0);//back to the root directory if any file operation has occurred
	//if(_fileWrite != 2) cd("/", 0);//back to the root directory if any file operation has occurred
	_filename[0] = '\0'; // put  NULL char at the first place in a name string
	_fileWrite = 0;
	_sectorCounter = 0;
	_cursorPos.value = 0;
	return tmpReturn;
}

////////////////////////////////////////////////////////////////
uint8_t Ch376msc::deleteFile(){
	if(!_deviceAttached) return 0x00;
	if(_interface == UARTT) {
		sendCommand(CMD_FILE_ERASE);
		_answer = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_ERASE);
		spiEndTransfer();
		_answer = spiWaitInterrupt();
	}
	cd("/",0);
	return _answer;
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

////	////	////	////	////	////	////	////
void Ch376msc::resetFileList(){
	fileProcesSTM = REQUEST;
}
///////////////////Listing files////////////////////////////
uint8_t Ch376msc::listDir(const char* filename){
/* __________________________________________________________________________________________________________
 * | 00 - 07 | 08 - 0A |  	0B     |     0C    |     0D     | 0E  -  0F | 10  -  11 | 12 - 13|  14 - 15 |
 * |Filename |Extension|File attrib|User attrib|First ch del|Create time|Create date|Owner ID|Acc rights|
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * | 16 - 17 | 18 - 19 |   1A - 1B   |  1C  -  1F |
 * |Mod. time|Mod. date|Start cluster|  File size |
 *
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
 * http://www.tavi.co.uk/phobos/fat.html
 */
	bool moreFiles = true;  // more files waiting for read out
	bool doneFiles = false; // done with reading a file

	while(!doneFiles){
		if(!_deviceAttached){
			moreFiles = false;
			break;
		}
		switch (fileProcesSTM) {
			case REQUEST:
				setFileName(filename);
				_answer = openFile();
				//_fileWrite = 2; // if in subdir
				fileProcesSTM = READWRITE;
				break;
			case READWRITE:
				if(_answer == ANSW_ERR_MISS_FILE){
					fileProcesSTM =DONE;
					moreFiles = false;// no more files in the directory
				}//end if
				if(_answer == ANSW_USB_INT_DISK_READ){
					rdFatInfo(); // read data to fatInfo buffer
					fileProcesSTM = NEXT;
				}
				break;
			case NEXT:
				_answer = fileEnumGo(); // go for the next filename
				fileProcesSTM = DONE;
				break;
			case DONE:
				if(!moreFiles){
					//closeFile(); // if no more files in the directory, close the file
					//closing file is not required after print dir (return value was always 0xB4 File is closed)
					fileProcesSTM = REQUEST;
				} else {
					fileProcesSTM = READWRITE;
				}
				doneFiles = true;
				break;
		}// end switch
	}//end while
	return moreFiles;
}

bool Ch376msc::readFileUntil(char trmChar, char* buffer, uint8_t b_num){//buffer for reading, buffer size
	char tmpBuff[2];//temporary buffer to read string and analyze
	bool readMore = true;
	uint8_t charCnt = 0;
	b_num --;// last byte is reserved for NULL terminating character
	if(!_deviceAttached) return false;
	for(; charCnt < b_num; charCnt ++){
		readMore = readFile(tmpBuff, sizeof(tmpBuff));
		buffer[charCnt] = tmpBuff[0];
		if((tmpBuff[0] == trmChar) || !readMore){// reach terminate character or EOF
			readMore = false;
			break;
		}
	}
	buffer[charCnt+1] = 0x00;//string terminate
	return readMore;
}

////////////////////////////  Read  cycle//////////////////////////
uint8_t Ch376msc::readFile(char* buffer, uint8_t b_num){ //buffer for reading, buffer size
	uint8_t tmpReturn = 0;// more data
	uint8_t byteForRequest ;
	bool bufferFull = false;
	_fileWrite = 0; // read mode, required for close procedure
	b_num--;// last byte is reserved for NULL terminating character
	if(_answer == ANSW_ERR_FILE_CLOSE || _answer == ANSW_ERR_MISS_FILE){
		bufferFull = true;
		tmpReturn = 0;// we have reached the EOF
	}
	while(!bufferFull){
		if(!_deviceAttached){
			tmpReturn = 0;
			break;
		}
		switch (fileProcesSTM) {
			case REQUEST:
				byteForRequest = b_num - _byteCounter;
				if(_sectorCounter == SECTORSIZE){ //if one sector has read out
					_sectorCounter = 0;
					fileProcesSTM = NEXT;
					break;
				} else if((_sectorCounter + byteForRequest) > SECTORSIZE){
					byteForRequest = SECTORSIZE - _sectorCounter;
				}
				////////////////
				_answer = reqByteRead(byteForRequest);
				if(_answer == ANSW_USB_INT_DISK_READ){
					fileProcesSTM = READWRITE;
					tmpReturn = 1; //we have not reached the EOF
				} else if(_answer == ANSW_USB_INT_SUCCESS){ // no more data, EOF
					fileProcesSTM = DONE;
					tmpReturn = 0;
				}
				break;
			case READWRITE:
				_sectorCounter += readDataToBuff(buffer);	//fillup the buffer
				if(_byteCounter != b_num) {
					fileProcesSTM = REQUEST;
				} else {
					fileProcesSTM = DONE;
				}
				break;
			case NEXT:
				_answer = byteRdGo();
				fileProcesSTM = REQUEST;
				break;
			case DONE:
				fileProcesSTM = REQUEST;
				buffer[_byteCounter] = '\0';// NULL terminating char
				_cursorPos.value += _byteCounter;
				_byteCounter = 0;
				bufferFull = true;
				break;
		}//end switch
	}//end while
		return tmpReturn;
}

///////////////////////////Write cycle/////////////////////////////

uint8_t Ch376msc::writeFile(char* buffer, uint8_t b_num){
	if(!_deviceAttached) return 0x00;
	_fileWrite = 1; // read mode, required for close procedure
	_byteCounter = 0;
	bool diskFree = true; //free space on a disk
	bool bufferFull = true; //continue to write while there is data in the temporary buffer
	if(_diskData.freeSector == 0){
		diskFree = false;
		return diskFree;
	}
	if(_answer == ANSW_ERR_MISS_FILE){ // no file with given name
		_answer = fileCreate();
	}//end if CREATED

	if(_answer == ANSW_ERR_FILE_CLOSE){
		_answer = openFile();
	}

	if(_answer == ANSW_USB_INT_SUCCESS){ // file created succesfully

		while(bufferFull){
			if(!_deviceAttached){
				diskFree = false;
				break;
			}
			switch (fileProcesSTM) {
				case REQUEST:
					_answer = reqByteWrite(b_num - _byteCounter);

					if(_answer == ANSW_USB_INT_SUCCESS){
						fileProcesSTM = NEXT;

					} else if(_answer == ANSW_USB_INT_DISK_WRITE){
						fileProcesSTM = READWRITE;
						}
					break;
				case READWRITE:
					writeDataFromBuff(buffer);
					if(_byteCounter != b_num) {
						fileProcesSTM = NEXT;
					} else {
						fileProcesSTM = DONE;
					}
					break;
				case NEXT:
					if(_diskData.freeSector > 0){
						_diskData.freeSector --;
						_answer = byteWrGo();
						if(_answer == ANSW_USB_INT_SUCCESS){
							fileProcesSTM = REQUEST;
						} else if(_byteCounter != b_num ){
							fileProcesSTM = READWRITE;
						}
					} else { // if disk is full
						fileProcesSTM = DONE;
						diskFree = false;
					}
					break;
				case DONE:
					fileProcesSTM = REQUEST;
					_cursorPos.value += _byteCounter;
					_byteCounter = 0;
					_answer = byteWrGo();
					bufferFull = false;
					break;
			}//end switch
		}//end while
	}// end file created

	return diskFree;
}

/////////////////////////////////////////////////////////////////
void Ch376msc::rdFatInfo(){
	uint8_t fatInfBuffer[32]; //temporary buffer for raw file FAT info
	uint8_t dataLength;
	if(_interface == UARTT){
		sendCommand(CMD_RD_USB_DATA0);
		dataLength = readSerDataUSB();
		for(uint8_t s =0;s < dataLength;s++){
			fatInfBuffer[s] = readSerDataUSB();// fill up temporary buffer
		}//end for
	} else {
		spiBeginTransfer();
		sendCommand(CMD_RD_USB_DATA0);
		dataLength = spiReadData();
		for(uint8_t s =0;s < dataLength;s++){
			fatInfBuffer[s] = spiReadData();// fill up temporary buffer
		}//end for
		spiEndTransfer();
	}
	memcpy ( &_fileData, &fatInfBuffer, sizeof(fatInfBuffer) ); //copy raw data to structured variable
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
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::readDataToBuff(char* buffer){
	uint8_t oldCounter = _byteCounter; //old buffer counter
	uint8_t dataLength; // data stream size
	if(_interface == UARTT) {
		sendCommand(CMD_RD_USB_DATA0);
		dataLength = readSerDataUSB(); // data stream size
		while(_byteCounter < (dataLength + oldCounter)){
			buffer[_byteCounter]=readSerDataUSB(); // incoming data add to buffer
			_byteCounter ++;
		}//end while
	} else {
	spiBeginTransfer();
	sendCommand(CMD_RD_USB_DATA0);
	dataLength = spiReadData(); // data stream size
	while(_byteCounter < (dataLength + oldCounter)){
		buffer[_byteCounter]=spiReadData(); // incoming data add to buffer
		_byteCounter ++;
	}//end while
	spiEndTransfer();
	}
	return dataLength;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::writeDataFromBuff(char* buffer){//====================
	uint8_t oldCounter = _byteCounter; //old buffer counter
	uint8_t dataLength; // data stream size
	if(_interface == UARTT) {
		sendCommand(CMD_WR_REQ_DATA);
		dataLength = readSerDataUSB(); // data stream size
	} else {
		spiBeginTransfer();
		sendCommand(CMD_WR_REQ_DATA);
		dataLength = spiReadData(); // data stream size
	}
	while(_byteCounter < (dataLength + oldCounter)){
		write(buffer[_byteCounter]); // read data from buffer and write to serial port
		_byteCounter ++;
	}//end while
	if(_interface == SPII) spiEndTransfer();
	return dataLength;
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
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::moveCursor(uint32_t position){
	if(!_deviceAttached) return 0x00;
	uint8_t tmpReturn = 0;
	//fSizeContainer _cursorPos; //unsigned long union
	if(position > _fileData.size){	//fix for moveCursor issue #3 Sep 17, 2019
		_sectorCounter = _fileData.size % SECTORSIZE;
	} else {
		_sectorCounter = position % SECTORSIZE;
	}
	_cursorPos.value = position;//temporary
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_BYTE_LOCATE);
	write(_cursorPos.b[0]);
	write(_cursorPos.b[1]);
	write(_cursorPos.b[2]);
	write(_cursorPos.b[3]);
	if(_interface == UARTT){
		tmpReturn = readSerDataUSB();
	} else {
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	if(_cursorPos.value > _fileData.size){
		_cursorPos.value = _fileData.size;//set the valid position
	}
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
void Ch376msc::sendFilename(){
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_SET_FILE_NAME);
	//write(0x2f); // "/" root directory
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
		setError(tmpReturn);
		memset(&_diskData, 0, sizeof(_diskData));// fill up with NULL disk data container
	}
	memcpy ( &_diskData, &tmpdata, sizeof(tmpdata) ); //copy raw data to structured variable
}

uint8_t Ch376msc::cd(const char* dirPath, bool mkDir){
	if(!_deviceAttached) return 0x00;
	uint8_t tmpReturn = 0;
	uint8_t pathLen = strlen(dirPath);
	_dirDepth = 0;
	if(pathLen < ((MAXDIRDEPTH*8)+(MAXDIRDEPTH+1)) ){//depth*(8char filename)+(directory separators)
		char input[pathLen + 1];
		strcpy(input,dirPath);
			  setFileName("/");
			  tmpReturn = openFile();
		char* command = strtok(input, "/");//split path into tokens
		  while (command != NULL){
			  if(strlen(command) > 8){//if a dir name is longer than 8 char
				  tmpReturn = ERR_LONGFILENAME;
				  break;
			  }
			  setFileName(command);
			  tmpReturn = openFile();
			  if(tmpReturn == ANSW_USB_INT_SUCCESS){//if file already exist with this name
				  tmpReturn = ANSW_ERR_FOUND_NAME;
				  closeFile();
				  break;
			  } else if(mkDir && (tmpReturn == ANSW_ERR_MISS_FILE)){
				  tmpReturn = dirCreate();
				  if(tmpReturn != ANSW_USB_INT_SUCCESS) break;
			  }//end if file exist
			  _dirDepth++;
			  command = strtok (NULL, "/");
		  }
	} else {
		tmpReturn = ERR_LONGFILENAME;
	}//end if path is to long
	return tmpReturn;
}

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


