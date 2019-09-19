/*
 * comm.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: György Kovács
 */


#include "Ch376msc.h"

uint8_t Ch376msc::readSerDataUSB(){
	uint32_t oldMillis = millis();
		while (!_comPort->available()){ // wait until data is arrive
			if ((millis()- oldMillis) > TIMEOUT){
				return 0xFF; // Timeout valasz
			}//end if
		}//end while
	return _comPort->read();
}

void Ch376msc::write(uint8_t data){
	if(_interface == UART){
		_comPort->write(data);
	} else { // SPI
		delayMicroseconds(3);//datasheet TSC min 1.5uSec
		spiReady();
			SPI.transfer(data);
		}
	}//end SPI

uint8_t Ch376msc::spiReadData(){
	delayMicroseconds(3);//datasheet TSC min 1.5uSec
	spiReady();
	return SPI.transfer(0x00);
}
void Ch376msc::print(const char str[]){
	uint8_t stringCounter = 0;
	if(_interface == UART){
		_comPort->print(str);
	} else { // SPI
		while(str[stringCounter]){ ///while not NULL
			write(str[stringCounter]);
			stringCounter++;
		}
	}
}

void Ch376msc::spiReady(){
	uint32_t msTimeout;
	delayMicroseconds(3);
	msTimeout = millis();
	while(digitalRead(_spiBusy)){
		if(millis()-msTimeout > TIMEOUT){
			break;
		}//end if
	}//end while
}

uint8_t Ch376msc::spiWaitInterrupt(){
	uint8_t answ = 0xFF;
	uint32_t oldMillis = millis();
	while(digitalRead(_intPin)){
		if ((millis()- oldMillis) > TIMEOUT){
			answ = 0;//timeout occurred
			break;
		}//end if
	}//end while
	if(answ){
		answ = getInterrupt();
	}
	return answ;
}

uint8_t Ch376msc::getInterrupt(){
	uint8_t _tmpReturn = 0;
	//spiWaitInterrupt();
		spiBeginTransfer();
		sendCommand(CMD_GET_STATUS);
		_tmpReturn = spiReadData();
		spiEndTransfer();
	return _tmpReturn;
}

void Ch376msc::spiBeginTransfer(){
	spiReady();
	SPI.beginTransaction(SPISettings(SPICLKRATE, MSBFIRST, SPI_MODE0));
	digitalWrite(_spiChipSelect, LOW);
}

void Ch376msc::spiEndTransfer(){
	digitalWrite(_spiChipSelect, HIGH);
	SPI.endTransaction();
}
