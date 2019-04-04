/*
 * Ch376msc.cpp
 *
 *  Created on: Feb 25, 2019
 *      Author: György Kovács
 *
 *
 */

#include "Ch376msc.h"

//with HW serial
Ch376msc::Ch376msc(HardwareSerial &usb,uint32_t speed) { // @suppress("Class members should be properly initialized")
	_comPortHW = &usb;
	_comPort = &usb;
	_speed = speed;
	_comPortHW->begin(9600);// start with default speed
	_byteCounter = 0;
	_ul_oldMillis = 0;
	_tmpReturn = 0;
	_mediaStatus = true;// true  because of init
	_answer = 0;
	_sectorCounter = 0;
	_controllerReady = false;
	_fileWrite = false;
	fileProcesSTM = REQUEST;
}
//with soft serial
Ch376msc::Ch376msc(Stream &sUsb) { // @suppress("Class members should be properly initialized")
	_comPort = &sUsb;
	_speed = 9600;
	_byteCounter = 0;
	_ul_oldMillis = 0;
	_tmpReturn = 0;
	_mediaStatus = false;// false because of init
	_answer = 0;
	_sectorCounter = 0;
	_controllerReady = false;
	_fileWrite = false;
	fileProcesSTM = REQUEST;


}

Ch376msc::~Ch376msc() {
	// TODO Auto-generated destructor stub
}
/////////////////////////////////////////////////////////////////
void Ch376msc::init(){
	reset();
	if(_mediaStatus){ // if Hardware serial is initialized
		setSpeed(); // Dynamically setup the com speed
		_mediaStatus = false;
	}
	_controllerReady = pingDevice();// eszkoz lekerdezes
	setMode(MODE_HOST);
	//mount();
	//adatUsbTol();
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
	_tmpReturn = 0;
	sendCommand(CMD_CHECK_EXIST);
	_comPort->write(0x01); // ez ertek negaltjat adja vissza
	if(adatUsbTol() == 0xFE){
		_tmpReturn = 1;//true
	}
	return _tmpReturn;
}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::setMode(uint8_t mode){
	sendCommand(CMD_SET_USB_MODE);
	_comPort->write(mode);
	_tmpReturn = adatUsbTol();
	_ul_oldMillis = millis();
	while(!_comPort->available()){
		//wait for the second byte 0x15 or 0x16 or timeout occurs
		if((millis()-_ul_oldMillis) > TIMEOUT){
			break;
		}
	}
	checkCH();

	return _tmpReturn; // success or fail
}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::mount(){ // return ANSWSUCCESS or ANSWFAIL
	sendCommand(CMD_DISK_MOUNT);
	_tmpReturn = adatUsbTol();
	return _tmpReturn;
}
/////////////////////////////////////////////////////////////////
bool Ch376msc::checkCH(){ //always call this function to you know is it any media attached to the usb
	if(_comPort->available()){
		while(_comPort->available()){ // while is needed, after connecting media, the ch376 send 3 message(connect, disconnect, connect)
			switch(adatUsbTol()){ // 0x15 ha csatlakoztatva, 0x16 ha nincs
			case ANSW_USB_INT_CONNECT:
				_mediaStatus = true;
				break;
			case ANSW_USB_INT_DISCONNECT:
				_mediaStatus = false;
				break;
			}//end switch
		}//end while
	} else if(!_mediaStatus){
		mount();
		while(_comPort->available()){ // while is needed, after connecting media, the ch376 send 3 message(connect, disconnect, connect)
			switch(adatUsbTol()){ // 0x15 ha csatlakoztatva, 0x16 ha nincs
			case ANSW_USB_INT_CONNECT:
				_mediaStatus = true;
				break;
			case ANSW_USB_INT_DISCONNECT:
				_mediaStatus = false;
				break;
			}//end switch
		}//end while
	}

	return _mediaStatus;
}
/////////////////Alap parancs kuldes az USB fele/////////////////
void Ch376msc::sendCommand(uint8_t b_parancs){
	_comPort->write(0x57);
	_comPort->write(0xAB);
	_comPort->write(b_parancs);
}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileOpen(){
	sendCommand(CMD_FILE_OPEN);
	_answer = adatUsbTol();
	if(_answer == ANSW_USB_INT_SUCCESS){ // get the file size
		/*
		sendCommand(CMD_GET_FILE_SIZE);
		_comPort->write(0x68);
		for(byte a = 0; a < 4; a++){
			_filesize.b[a] = adatUsbTol();
		}*/
		dirInfoRead();
	}
	return _answer;
}

uint8_t Ch376msc::dirInfoRead(){
	sendCommand(CMD_DIR_INFO_READ);// max 16 files 0x00 - 0x0f
	_comPort->write(0xff);// current file is 0xff
	adatUsbTol();
	rdUsbData();
	return false;
}

uint8_t Ch376msc::dirInfoSave(){
	_fileWrite = true;
	sendCommand(CMD_DIR_INFO_READ);
	_comPort->write(0xff);// current file is 0xff
		adatUsbTol();
	writeFatData();//send fat data
	sendCommand(CMD_DIR_INFO_SAVE);
	return adatUsbTol();
}

void Ch376msc::writeFatData(){// see fat info table under next filename
	uint8_t fatInfBuffer[32]; //temporary buffer for raw file FAT info
	memcpy ( &fatInfBuffer, &_fileData,  sizeof(fatInfBuffer) ); //copy raw data to temp buffer
	sendCommand(CMD_WR_OFS_DATA);
	_comPort->write((uint8_t)0x00);
	_comPort->write(32);
	for(uint8_t d = 0;d < 32; d++){
		_comPort->write(fatInfBuffer[d]);
		//address++;
	}
}
////////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileClose(){ // 0x00 - frissites nelkul, 0x01 adatmeret frissites
	uint8_t d = 0x00;
	if(_fileWrite){ // if closing file after write procedure
		//byteWrGo();	// request
		d = 0x01; // close with 0x01 (to update file length)
		//reqByteWrite(0x00);
	}
	sendCommand(CMD_FILE_CLOSE);
	_comPort->write(d);
	_filename[0] = '\0'; // put  NULL char at the first place in a name string
	//_filesize.value = 0;
	_fileWrite = false;
	_sectorCounter = 0;
	return adatUsbTol();
}
////////////////////////////////////////////////////////////////
uint8_t Ch376msc::deleteFile(){
	sendCommand(CMD_FILE_ERASE);
	_answer = adatUsbTol();
	return _answer;
}
///////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileEnumGo(){
	sendCommand(CMD_FILE_ENUM_GO);
	return adatUsbTol();
}
//////////////////////////////////////////////////////////////
uint8_t Ch376msc::byteRdGo(){
	sendCommand(CMD_BYTE_RD_GO);
	return adatUsbTol();
}
//////////////////////////////////////////////////////////////
uint8_t Ch376msc::fileCreate(){
	//_filesize.value = 0;//
	sendCommand(CMD_FILE_CREATE);
	return adatUsbTol();
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
	_tmpReturn = 1; // more files waiting for read out
	bool doneFiles = false; // done with reading a file
	while(!doneFiles){
		switch (fileProcesSTM) {
			case REQUEST:
				strcpy(_filename,"*");// select all
				sendFilename();
				_answer = fileOpen();
				_fileWrite = false; // read mode
				fileProcesSTM = READWRITE;
				break;
			case READWRITE:
				if(_answer == ANSW_ERR_MISS_FILE){
					fileProcesSTM =DONE;
					_tmpReturn = 0; // no more files in the directory
				}//end if
				if(_answer == ANSW_USB_INT_DISK_READ){
					rdUsbData(); // read data to fatInfo buffer
					/*
					strncpy(nameBuff[elemSzamlalo],_fileReszletekOsszes.name,11); // copy file name
					strcat(nameBuff[elemSzamlalo],"\0"); //concatenate  NULL char
					sizeBuff[elemSzamlalo] = _fileReszletekOsszes.meret; // get the file size
					elemSzamlalo++;
					*/
					fileProcesSTM = NEXT;
				}
				break;
			case NEXT:
				_answer = fileEnumGo(); // go for the next filename
				fileProcesSTM = DONE;
				break;
			case DONE:
				if(!_tmpReturn){
					fileClose(); // if no more files in the directory, close the file
					fileProcesSTM = REQUEST;
				} else {
					fileProcesSTM = READWRITE;
				}

				doneFiles = true;
				break;
		}// end switch
	}//end while
	return _tmpReturn;
}
////////////////////////////  Read  cycle//////////////////////////
uint8_t Ch376msc::readFile(char* buffer, uint8_t b_num){ //buffer for reading, buffer size
	uint8_t byteForRequest ;
	bool bufferFull = false;
	_fileWrite = false;
	b_num--;// last byte is reserved for NULL terminating character
	_tmpReturn = 0; // more data
	_byteCounter = 0;
	if(_answer == ANSW_ERR_FILE_CLOSE || _answer == ANSW_ERR_MISS_FILE){
		bufferFull = true;
		_tmpReturn = 0;// we have reached the EOF
	}
	while(!bufferFull){

		switch (fileProcesSTM) {
			case REQUEST:
				byteForRequest = b_num - _byteCounter;
				if(_sectorCounter == 512){ //if one sector has read out
					_sectorCounter = 0;
					fileProcesSTM = NEXT;
					break;
				} else if((_sectorCounter + byteForRequest) > 512){
					byteForRequest = 512 - _sectorCounter;
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
// 77ms - 168ms with 196 char long buffer, little bit slow
uint8_t Ch376msc::writeFile(char* buffer, uint8_t b_num){
	_fileWrite = true;
	_byteCounter = 0;
	_tmpReturn = 0; //ready for next chunk of data
	if(_answer == ANSW_ERR_MISS_FILE){ // no file with given name
		_answer = fileCreate();
	}//end if CREATED

	if(_answer == ANSW_ERR_FILE_CLOSE){
		_answer = fileOpen();
	}

	if(_answer == ANSW_USB_INT_SUCCESS){ // file created succesfully

		while(!_tmpReturn){
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
					_answer = byteWrGo();
					if(_answer == ANSW_USB_INT_SUCCESS){
						fileProcesSTM = REQUEST;
					} else if(_byteCounter != b_num ){
						fileProcesSTM = READWRITE;
					}
					break;
				case DONE:
					fileProcesSTM = REQUEST;
					_byteCounter = 0;
					_answer = byteWrGo();
					_tmpReturn = true;
					break;
			}//end switch
		}//end while
	}// end file created

	return _tmpReturn;
}

/////////////////////////////////////////////////////////////////
void Ch376msc::rdUsbData(){
	uint8_t fatInfBuffer[32]; //temporary buffer for raw file FAT info
	sendCommand(CMD_RD_USB_DATA0);
	uint8_t adatHossz = adatUsbTol();/// ALWAYS 32 byte, otherwise kaboom
		for(uint8_t s =0;s < adatHossz;s++){
			fatInfBuffer[s] = adatUsbTol();// fillup temp buffer
		}
	memcpy ( &_fileData, &fatInfBuffer, sizeof(fatInfBuffer) ); //copy raw data to structured variable

}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::byteWrGo(){
	sendCommand(CMD_BYTE_WR_GO);
	return adatUsbTol();
}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::readDataToBuff(char* buffer){
	sendCommand(CMD_RD_USB_DATA0);
	uint8_t regiSzamlalo = _byteCounter; //old buffer counter
	uint8_t adatHossz = adatUsbTol(); // data stream size
	while(_byteCounter < (adatHossz + regiSzamlalo)){
		buffer[_byteCounter]=adatUsbTol(); // incoming data add to buffer
		_byteCounter ++;
	}//end while
	return adatHossz;
}
/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::writeDataFromBuff(char* buffer){//====================
	sendCommand(CMD_WR_REQ_DATA);
	uint8_t regiSzamlalo = _byteCounter; //old buffer counter
	uint8_t b_adatHossz = adatUsbTol(); // data stream size
	while(_byteCounter < (b_adatHossz + regiSzamlalo)){
		_comPort->write(buffer[_byteCounter]); // read data from buffer and write to serial port
		_byteCounter ++;
		//_filesize.value ++;
	}//end while
	//delayMicroseconds(10);
	return b_adatHossz;
}

/////////////////////////////////////////////////////////////////
uint8_t Ch376msc::reqByteRead(uint8_t a){
	sendCommand(CMD_BYTE_READ);
	_comPort->write(a); // request data stream length for reading, 00 - FF
	_comPort->write((uint8_t)0x00);
	return adatUsbTol();
}
////////////////////////////////////////////////////////////////
uint8_t Ch376msc::reqByteWrite(uint8_t a){
	sendCommand(CMD_BYTE_WRITE);
	_comPort->write(a); // request data stream length for writing, 00 - FF
	_comPort->write((uint8_t)0x00);
	return adatUsbTol();
}
uint8_t Ch376msc::moveCursor(uint32_t position){
	fSizeContainer cPosition; //unsigned long union
	cPosition.value = position;
	sendCommand(CMD_BYTE_LOCATE);
	_comPort->write(cPosition.b[0]);
	_comPort->write(cPosition.b[1]);
	_comPort->write(cPosition.b[2]);
	_comPort->write(cPosition.b[3]);

	return adatUsbTol();
}
//////////////USB tol bejovo byte ////////////////////
uint8_t Ch376msc::adatUsbTol(){
	uint8_t valasz = 0xFF;
	_ul_oldMillis = millis();
	while (!_comPort->available()){ // wait until data is arrive
		if ((millis()-_ul_oldMillis) > TIMEOUT){
			return valasz; // Timeout valasz
		}
	}
	return _comPort->read();
}
/////////////////////////////////////////////////////
void Ch376msc::sendFilename(){
	sendCommand(CMD_SET_FILE_NAME);
	_comPort->write(0x2f); // "/" root directory
	_comPort->print(_filename); // filename
	_comPort->write(0x5C);	// ez a "\" jel
	_comPort->write((uint8_t)0x00);	// ez a lezaro 0 jel
	//delay(5);
}
////////////////////////////////////////////////////
void Ch376msc::reset(){
	sendCommand(CMD_RESET_ALL);
	delay(60);// wait after reset command, according to the datasheet 35ms is required, but that was too short
}

