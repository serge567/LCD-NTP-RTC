/*
  Copyright (c) 2010 Cibeira Gerardo
*/

#ifndef MBED_DS1302_H
#define MBED_DS1302_H

#include "mbed.h"
#define BYTE unsigned char

class ds1302
{
public:
    //constructor
    ds1302(PinName pinSCLK,PinName pinIO,PinName pinRST);
    
    //methods
    void write_byte(BYTE cmd);
    void write(BYTE cmd, BYTE data);
    BYTE read(BYTE cmd);
    void init();
    BYTE get_bcd(BYTE data);
    BYTE rm_bcd(BYTE data);
    void set_datetime(BYTE day, BYTE mth, BYTE year, BYTE dow, BYTE hr, BYTE min, BYTE sec);
    void get_date(BYTE &day, BYTE &mth, BYTE &year, BYTE &dow);
    void get_time(BYTE &hr, BYTE &min, BYTE &sec);
    void write_nvr(BYTE address, BYTE data);
    BYTE read_nvr(BYTE address);
    
    
private:
    //data
    DigitalOut SCLK;
    DigitalInOut IO;
    DigitalOut RST;
};

#endif