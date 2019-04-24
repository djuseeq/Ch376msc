# Project Title

Arduino library for CH376 Mass Storage Contoller

## Getting Started
Configure jumpers on the module

![Alt text](pic/JumperSelect.png?raw=true "Setting")

## Versioning
v1.2.1     Apr 24, 2019 - In use of SPI, CS pin on the module must to be pulled to VCC otherwise communication can be instable on a higher clock rate
- Fixed timing issue on a higher clock rate (TSC)
                  
v1.2 Apr 20, 2019 -extended with SPI communication

v1.1 Feb 25, 2019 -initial version with UART communication

### Acknowledgments

Thanks for the idea to Scott C ,  https://arduinobasics.blogspot.com/2015/05/ch376s-usb-readwrite-module.html
