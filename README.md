# Arduino library for CH376 file manage control chip
Support read/write files on USB flash drive. The chip support FAT16, FAT32 and FAT12 file system

## Getting Started
Configure the jumpers on the module depending on which communication protocol you are using

![Alt text](extras/JumperSelect.png?raw=true "Setting")

## Versioning
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

### Acknowledgments

Thanks for the idea to Scott C ,  https://arduinobasics.blogspot.com/2015/05/ch376s-usb-readwrite-module.html

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
