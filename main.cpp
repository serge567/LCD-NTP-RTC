#include "mbed.h"
#include "ds1302.h" // RTC module
#include "EthernetInterface.h" // NTP
#include "ntp-client/NTPClient.h" // NTP

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
    EthernetInterface eth;
    eth.connect();
    // Show the network address
    SocketAddress a;
    eth.get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");
    
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

int main() 
{
    //DS1302 RTC Module -->
    clk.init();
    RTC_PrintDateTime();
    RTC_Synch();
    RTC_PrintDateTime();
    // <-- DS1302 RTC Module
}

