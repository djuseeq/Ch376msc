/*
 * Ch376msc.cpp
 *
 *  Created on: Feb 25, 2019
 *      Author: György Kovács
 *
 *
 *
 */

#include "Ch376msc.h"

//with HW serial
Ch376msc::Ch376msc(HardwareSerial &usb, uint32_t speed) { // @suppress("Class members should be properly initialized")
	_interface = UART;
	_comPortHW = &usb;
	_comPort = &usb;
	_speed = speed;
	_hwSerial = true;
}
//with soft serial
Ch376msc::Ch376msc(Stream &sUsb) { // @suppress("Class members should be properly initialized")
	_interface = UART;
	_comPort = &sUsb;
	_speed = 9600;
	_hwSerial = false;
}

//with SPI, MISO as INT pin(the SPI is only available for CH376)
Ch376msc::Ch376msc(uint8_t spiSelect, uint8_t busy){ // @suppress("Class members should be properly initialized")
	_interface = SPII;
	_intPin = MISO; // use the SPI MISO for interrupt JUST if no other device is using the SPI!!
	_spiChipSelect = spiSelect;
	_spiBusy = busy;
	_speed = 0;
}
//not tested wit other lib
Ch376msc::Ch376msc(uint8_t spiSelect, uint8_t busy, uint8_t intPin){ // @suppress("Class members should be properly initialized")
	_interface = SPII;
	_intPin = intPin;
	_spiChipSelect = spiSelect;
	_spiBusy = busy;
	_speed = 0;
}

Ch376msc::~Ch376msc() {
	//  Auto-generated destructor stub
}

/////////////////////////////////////////////////////////////////
void Ch376msc::init(){
	delay(100);//wait for VCC to normalize
	if(_interface == SPII){
		pinMode(_spiChipSelect, OUTPUT);
		digitalWrite(_spiChipSelect, HIGH);
		pinMode(_spiBusy, INPUT);
		if(_intPin != MISO) pinMode(_intPin, INPUT_PULLUP);
		SPI.begin();
		spiBeginTransfer();
		sendCommand(CMD_RESET_ALL);
		spiEndTransfer();
		delay(40);
		spiReady();//wait for device
		if(_intPin == MISO){ // if we use MISO as interrupt pin, then tell it for the device ;)
			spiBeginTransfer();
			sendCommand(CMD_SET_SD0_INT);
			write(0x16);
			write(0x90);
			spiEndTransfer();
		}//end if
	} else {//UART
		if(_hwSerial) _comPortHW->begin(9600);// start with default speed
		sendCommand(CMD_RESET_ALL);
		delay(60);// wait after reset command, according to the datasheet 35ms is required, but that was too short
		if(_hwSerial){ // if Hardware serial is initialized
			setSpeed(); // Dynamically setup the com speed
		}
	}//end if UART
	_controllerReady = pingDevice();// check the communication
	setMode(MODE_HOST_1);
	checkDrive();
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
	uint8_t _tmpReturn = 0;
	if(_interface == UART){
		sendCommand(CMD_CHECK_EXIST);
		write(0x01); // ez ertek negaltjat adja vissza
		if(readSerDataUSB() == 0xFE){
			_tmpReturn = 1;//true
		}
	} else {
		spiBeginTransfer();
		sendCommand(CMD_CHECK_EXIST);
		write(0x01); // ez ertek negaltjat adja vissza
		if(spiReadData() == 0xFE){
			_tmpReturn = 1;//true
		}
		spiEndTransfer();
	}
	return _tmpReturn;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::setMode(uint8_t mode){
	uint8_t _tmpReturn = 0;
	uint32_t oldMillis;
	if(_interface == UART){
		sendCommand(CMD_SET_USB_MODE);
		write(mode);
		_tmpReturn = readSerDataUSB();
		oldMillis = millis();
		while(!_comPort->available()){
			//wait for the second byte 0x15 or 0x16 or timeout occurs
			if((millis()- oldMillis) > TIMEOUT){
				break;
			}
		}
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_SET_USB_MODE);
		write(mode);
		_tmpReturn = spiReadData();
		spiEndTransfer();
		delayMicroseconds(40);
	}
	checkDrive();
	return _tmpReturn; // success or fail
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::mount(){ // return ANSWSUCCESS or ANSWFAIL
	uint8_t _tmpReturn = 0;
	if(_interface == UART) {
		sendCommand(CMD_DISK_MOUNT);
		_tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_DISK_MOUNT);
		spiEndTransfer();
		_tmpReturn = getInterrupt();
	}
	return _tmpReturn;
}

/////////////////////////////////////////////////////////////////
bool Ch376msc::checkDrive(){ //always call this function to you know is it any media attached to the usb
	uint8_t _tmpReturn = 0;
		if(_interface == UART){
			while(_comPort->available()){ // while is needed, after connecting media, the ch376 send 3 message(connect, disconnect, connect)
				_tmpReturn = readSerDataUSB();
			}//end while
		} else {//spi
			if(!digitalRead(_intPin)){
				_tmpReturn = getInterrupt(); // get int message
			}//end if int message pending
		}
		switch(_tmpReturn){ // 0x15 device attached, 0x16 device disconnect
		case ANSW_USB_INT_CONNECT:
			_deviceAttached = true;
			rdDiskInfo();
			break;
		case ANSW_USB_INT_DISCONNECT:
			_deviceAttached = false;
			memset(&_diskData, 0, sizeof(_diskData));// fill up with NULL disk data container
			break;
		}//end switch
	return _deviceAttached;
}

/////////////////Alap parancs kuldes az USB fele/////////////////
void Ch376msc::sendCommand(uint8_t b_parancs){
	if(_interface == UART){
	write(0x57);// UART first sync command
	write(0xAB);// UART second sync command
	}//end if
	write(b_parancs);
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::openFile(){
	if(_interface == UART){
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
	uint8_t _tmpReturn;
	if(_interface == UART){
		sendCommand(CMD_DIR_INFO_READ);// max 16 files 0x00 - 0x0f
		write(0xff);// current file is 0xff
		_tmpReturn = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_DIR_INFO_READ);// max 16 files 0x00 - 0x0f
		write(0xff);// current file is 0xff
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	rdFatInfo();
	return _tmpReturn;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::dirInfoSave(){
	uint8_t _tmpReturn = 0;
	_fileWrite = true;
	if(_interface == UART) {
		sendCommand(CMD_DIR_INFO_READ);
		write(0xff);// current file is 0xff
		readSerDataUSB();
		writeFatData();//send fat data
		sendCommand(CMD_DIR_INFO_SAVE);
		_tmpReturn = readSerDataUSB();
	} else {//spi
		spiBeginTransfer();
		sendCommand(CMD_DIR_INFO_READ);
		write(0xff);// current file is 0xff
		spiEndTransfer();
		getInterrupt();
		writeFatData();//send fat data
		spiBeginTransfer();
		sendCommand(CMD_DIR_INFO_SAVE);
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	return _tmpReturn;
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
		//address++;
	}
	if(_interface == SPII) spiEndTransfer();
}

////////////////////////////////////////////////////////////////
uint8_t Ch376msc::closeFile(){ // 0x00 - frissites nelkul, 0x01 adatmeret frissites
	uint8_t _tmpReturn = 0;
	uint8_t d = 0x00;
	if(_fileWrite){ // if closing file after write procedure
		d = 0x01; // close with 0x01 (to update file length)
	}
	if(_interface == UART){
		sendCommand(CMD_FILE_CLOSE);
		write(d);
		_tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_CLOSE);
		write(d);
		//read();
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	memset(&_fileData, 0, sizeof(_fileData));// fill up with NULL file data container
	_filename[0] = '\0'; // put  NULL char at the first place in a name string
	_fileWrite = false;
	_sectorCounter = 0;
	return _tmpReturn;
}

////////////////////////////////////////////////////////////////
uint8_t Ch376msc::deleteFile(){
	if(_interface == UART) {
		sendCommand(CMD_FILE_ERASE);
		_answer = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_ERASE);
		spiEndTransfer();
		_answer = spiWaitInterrupt();
	}
	return _answer;
}

///////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileEnumGo(){
	uint8_t _tmpReturn = 0;
	if(_interface == UART){
		sendCommand(CMD_FILE_ENUM_GO);
		_tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_ENUM_GO);
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	return _tmpReturn;
}

//////////////////////////////////////////////////////////////
uint8_t Ch376msc::byteRdGo(){
	uint8_t _tmpReturn = 0;
	if(_interface == UART) {
		sendCommand(CMD_BYTE_RD_GO);
		_tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_BYTE_RD_GO);
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	return _tmpReturn;
}

//////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileCreate(){
	uint8_t _tmpReturn = 0;
	if(_interface == UART) {
		sendCommand(CMD_FILE_CREATE);
		_tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_FILE_CREATE);
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	return _tmpReturn;
}

////	////	////	////	////	////	////	////
///////////////////Listing files////////////////////////////
uint8_t Ch376msc::listDir(){
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
		switch (fileProcesSTM) {
			case REQUEST:
				setFileName("*");
				sendFilename();
				_answer = openFile();
				_fileWrite = false; // read mode, required for close procedure
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
					closeFile(); // if no more files in the directory, close the file
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

////////////////////////////  Read  cycle//////////////////////////
uint8_t Ch376msc::readFile(char* buffer, uint8_t b_num){ //buffer for reading, buffer size
	uint8_t _tmpReturn = 0;// more data
	uint8_t byteForRequest ;
	bool bufferFull = false;
	_fileWrite = false; // read mode, required for close procedure
	b_num--;// last byte is reserved for NULL terminating character
	if(_answer == ANSW_ERR_FILE_CLOSE || _answer == ANSW_ERR_MISS_FILE){
		bufferFull = true;
		_tmpReturn = 0;// we have reached the EOF
	}
	while(!bufferFull){

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
					_tmpReturn = 1; //we have not reached the EOF
				} else if(_answer == ANSW_USB_INT_SUCCESS){ // no more data, EOF
					fileProcesSTM = DONE;
					_tmpReturn = 0;
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
				_byteCounter = 0;
				bufferFull = true;
				break;
		}//end switch
	}//end while
		return _tmpReturn;
}

///////////////////////////Write cycle/////////////////////////////

uint8_t Ch376msc::writeFile(char* buffer, uint8_t b_num){
	_fileWrite = true; // read mode, required for close procedure
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
	if(_interface == UART){
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
	uint8_t _tmpReturn = 0;
	if(_interface == UART) {
		sendCommand(CMD_BYTE_WR_GO);
		_tmpReturn = readSerDataUSB();
	} else {
		spiBeginTransfer();
		sendCommand(CMD_BYTE_WR_GO);
		spiEndTransfer();
		_tmpReturn = spiWaitInterrupt();
	}
	return _tmpReturn;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::readDataToBuff(char* buffer){
	uint8_t oldCounter = _byteCounter; //old buffer counter
	uint8_t dataLength; // data stream size
	if(_interface == UART) {
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
	if(_interface == UART) {
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
	if(_interface == UART){
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
	if(_interface == UART) {
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
	uint8_t tmpReturn = 0;
	fSizeContainer cPosition; //unsigned long union
	if(position > _fileData.size){	//fix for moveCursor issue #3 Sep 17, 2019
		_sectorCounter = _fileData.size % SECTORSIZE;
	} else {
		_sectorCounter = position % SECTORSIZE;
	}
	cPosition.value = position;
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_BYTE_LOCATE);
	write(cPosition.b[0]);
	write(cPosition.b[1]);
	write(cPosition.b[2]);
	write(cPosition.b[3]);
	if(_interface == UART){
		tmpReturn = readSerDataUSB();
	} else {
		spiEndTransfer();
		tmpReturn = spiWaitInterrupt();
	}
	return tmpReturn;
}

/////////////////////////////////////////////////////////////////
void Ch376msc::sendFilename(){
	if(_interface == SPII) spiBeginTransfer();
	sendCommand(CMD_SET_FILE_NAME);
	write(0x2f); // "/" root directory
	print(_filename); // filename
	write(0x5C);	// ez a "\" jel
	write((uint8_t)0x00);	// ez a lezaro 0 jel
	if(_interface == SPII) spiEndTransfer();
}
/////////////////////////////////////////////////////////////////
void Ch376msc::rdDiskInfo(){
	uint8_t dataLength;
	uint8_t tmpReturn;
	uint8_t tmpdata[9];
	if(_interface == UART){
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
	memcpy ( &_diskData, &tmpdata, sizeof(tmpdata) ); //copy raw data to structured variable
}
