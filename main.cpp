#include "mbed.h"
#include "ds1302.h" // RTC module
#include "EthernetInterface.h" // NTP
#include "ntp-client/NTPClient.h" // NTP
#include "TextLCD.h" // LCD 1602 with PCF8574, I2C bus
#include <string>
 
//LCD 1602
I2C i2c_lcd(D14,D15); // SDA, SCL
// 1001110  - 0x4E
// 100111	- 0x27
TextLCD_I2C lcd(&i2c_lcd, 0x4E, TextLCD::LCD16x2); // I2C bus, PCF8574 Slaveaddress, LCD Type
Ticker SecTick;

// NTP 
time_t timestamp;

// RTC module ->
#define SCLK    D13 // CLK
#define IO      D12 // DATA
#define CE      D8 // RST
const char *swd[8] = { "", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }; 
const char *smth[13] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; 
struct tm * timeinfo;
unsigned char day;
unsigned char mth;
unsigned char year;
unsigned char dow;
unsigned char hr;
unsigned char minu;
unsigned char sec;
ds1302 clk(SCLK, IO, CE);
// <- RTC module

// Interrupts
bool bOneSecTick = false;

void RTC_PrintDateTime()
{
    clk.get_date(day, mth, year, dow);
    clk.get_time(hr, minu, sec);
    printf("RTC module date and time: %s, %s %d, %d  %d:%d:%d \r\n", swd[dow], smth[mth], day, year + 2000, hr, minu, sec);   
}

void RTC_Synch()
{
     //NTP Client ->
    printf("NTP Client example (using Ethernet)\r\n");
    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("%s", "Connect to NTP"); 
    lcd.locate(0, 1);
    lcd.printf("%s", "  Server");
    EthernetInterface eth;
    eth.connect();
    // Show the network address
    SocketAddress a;
    eth.get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");
    ThisThread::sleep_for(1s);
    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("%s", "Interface IP"); 
    lcd.locate(0, 1);
    lcd.printf("%s", a.get_ip_address() ? a.get_ip_address() : "None"); 
    ThisThread::sleep_for(5s);
    NTPClient ntp(&eth);
    
    timestamp = ntp.get_timestamp();
    timestamp = timestamp + 3600 * 9; // Japan Time +9 hours offset
    if (timestamp > 3820313209) 
    {
        printf("An error occurred when getting the time. Timestamp %u sec since 1900 must be > 3820313209\r\n", timestamp);
    } 
    else 
    {
        printf("Current time is %s\r\n", ctime(&timestamp)); 
        timeinfo    = localtime(&timestamp);
        day         = (unsigned char) timeinfo -> tm_mday;
        mth         = (unsigned char) (timeinfo -> tm_mon + 1);
        year        = (unsigned char) (timeinfo -> tm_year + 1900 - 2000);
        dow         = (unsigned char) (timeinfo -> tm_wday + 1);
        hr          = (unsigned char) timeinfo -> tm_hour;
        minu        = (unsigned char) timeinfo -> tm_min;
        sec         = (unsigned char) timeinfo -> tm_sec;
        clk.set_datetime(day, mth, year, dow, hr, minu, sec);
        printf("RTC module date and time: %s, %s %d, %d  %d:%d:%d synchronized with NTP sertver.\r\n", swd[dow], smth[mth], day, year + 2000, hr, minu, sec);   
    }
}

string StrLeadZero(unsigned char number)
{
    string snumber = to_string(number);
    if (snumber.length() == 1)
    { snumber = "0" + snumber; }
    return snumber;
}

void LCDDisplayTime()
{
    clk.get_date(day, mth, year, dow);
    clk.get_time(hr, minu, sec);
    lcd.locate(0, 0);
    lcd.printf("%s %s %d, %d ", swd[dow], smth[mth], day, year + 2000); 
    lcd.locate(0, 1);    
    lcd.printf(" %s:%s:%s ", StrLeadZero(hr).c_str(), StrLeadZero(minu).c_str(), StrLeadZero(sec).c_str());
}

void fSecondTick()
{
    bOneSecTick = true; 
    // avoid here to input instructions which required long execution time such as printf ..
}

int main() 
{
    //LCD 1602
    lcd.cls();
    lcd.setBacklight(TextLCD::LightOn);

    //DS1302 RTC Module -->
    clk.init();
    RTC_PrintDateTime();
    RTC_Synch();
    RTC_PrintDateTime();
    // <-- DS1302 RTC Module  
    
    //LCD 1602
    lcd.cls(); 

    SecTick.attach(&fSecondTick, 1000ms); 
    while(1)
    {
     //  ThisThread::sleep_for(1s);
     if (bOneSecTick)
     {
        bOneSecTick = false;
        LCDDisplayTime();
     }
    }
}
