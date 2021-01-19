/*
  Copyright (c) 2010 Cibeira Gerardo
*/

#include "ds1302.h"
#include "mbed.h"

//constructor
ds1302::ds1302(PinName pinSCLK,PinName pinIO,PinName pinRST) : SCLK(pinSCLK),IO(pinIO),RST(pinRST) {}

//methods
void ds1302::write_byte(BYTE cmd)
{
    BYTE i;
    IO.output();
    for(i=0;i<=7;i++)
    {
        IO = (cmd >> i) & 0x01;
        wait_us(1);
        SCLK=1;
        wait_us(1);
        SCLK=0;
    }
    IO.input();
}

void ds1302::write(BYTE cmd, BYTE data)
{
    RST=1;
    wait_us(1);
    write_byte(cmd);
    write_byte(data);
    RST=0;
}

BYTE ds1302::read(BYTE cmd)
{
    BYTE i,data=0;
    
    RST=1;
    write_byte(cmd);
    
    IO.input();
    wait_us(1);
    
    for(i=0;i<=7;i++)
    {
       data += IO<<i;
       SCLK=1;
       wait_us(1);
       SCLK=0;
       wait_us(1);
    }
    RST=0;
    
    return(data);
}

void ds1302::init()
{
    BYTE x;
    RST=0;
    wait_us(2);
    SCLK=0;
    write(0x8e,0);
    write(0x90,0xa4);
    x=read(0x81);
    if((x & 0x80)!=0)
        write(0x80,0);
}

BYTE ds1302::get_bcd(BYTE data)
{
    BYTE nibh=0;
    BYTE nibl=0;
    
    nibh = data/10;
    nibl = data-(nibh*10);
    
    return( (nibh<<4) | nibl);
}

BYTE ds1302::rm_bcd(BYTE data)
{
    BYTE i,aux=0;
    
    i = data;
    aux = (i>>4)*10;
    aux += (i & 0x0F);
    
    return aux;
}

void ds1302::set_datetime(BYTE day, BYTE mth, BYTE year, BYTE dow, BYTE hr, BYTE min, BYTE sec)
{
    write(0x86,get_bcd(day));
    write(0x88,get_bcd(mth));
    write(0x8c,get_bcd(year));
    write(0x8a,get_bcd(dow));
    write(0x84,get_bcd(hr));
    write(0x82,get_bcd(min));
    //write(0x80,get_bcd(0));
    write(0x80,get_bcd(sec));
}

void ds1302::get_date(BYTE &day, BYTE &mth, BYTE &year, BYTE &dow)
{
    day = rm_bcd(read(0x87));
    mth = rm_bcd(read(0x89));
    year = rm_bcd(read(0x8d));
    dow = rm_bcd(read(0x8b));
}

void ds1302::get_time(BYTE &hr, BYTE &min, BYTE &sec)
{
    hr = rm_bcd(read(0x85));
    min = rm_bcd(read(0x83));
    sec = rm_bcd(read(0x81));
}

void ds1302::write_nvr(BYTE address, BYTE data)
{
    write(address|0xc0,data);
}

BYTE ds1302::read_nvr(BYTE address)
{
     return(read(address|0xc1));
}