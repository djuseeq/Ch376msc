/*
 * comm.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: djusee
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
		//delayMicroseconds(3);
			SPI.transfer(data);
		}
	}//end SPI


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

void Ch376msc::waitSpiInterrupt(){
	uint32_t oldMillis = millis();
	while(digitalRead(_intPin)){
		if ((millis()- oldMillis) > TIMEOUT){
			break;
		}//end if
	}//end while
}

uint8_t Ch376msc::getInterrupt(){
	uint8_t _tmpReturn = 0;
	waitSpiInterrupt();
		spiBeginTransfer();
		sendCommand(CMD_GET_STATUS);
		_tmpReturn = SPI.transfer(0x00);
		spiEndTransfer();
	return _tmpReturn;
}

void Ch376msc::spiBeginTransfer(){
	SPI.beginTransaction(SPISettings(SPICLKRATE, MSBFIRST, SPI_MODE0));
	digitalWrite(_spiChipSelect, LOW);
}

void Ch376msc::spiEndTransfer(){
	digitalWrite(_spiChipSelect, HIGH);
	SPI.endTransaction();
}
