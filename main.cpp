#include "mbed.h"
#include "ds1302.h" // RTC module
#include "EthernetInterface.h" // NTP
#include "ntp-client/NTPClient.h" // NTP

// RTC module pins
#define SCLK    D13 // CLK
#define IO      D12 // DATA
#define CE      D8 // RST
ds1302 clk(SCLK, IO, CE);

int main() 
{
    //DS1302 RTC Module -->
    const char *swd[8] = { "Undef", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    clk.init();
    bool bSynClock = false;
    struct tm * timeinfo;
    unsigned char day;
    unsigned char mth;
    unsigned char year;
    unsigned char dow;
    unsigned char hr;
    unsigned char min;
    unsigned char sec;
    clk.get_date(day, mth, year, dow);
    clk.get_time(hr, min, sec);
    printf("RTC module date and time: %s %d/%d/%d  %d:%d:%d \r\n", swd[dow], day, mth, year, hr, min, sec);
    // <-- DS1302 RTC Module

    printf("NTP Client example (using Ethernet)\r\n");
    EthernetInterface eth;
    eth.connect();
    // Show the network address
    SocketAddress a;
    eth.get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");
    
    NTPClient ntp(&eth);
    
    while(1) 
    {
        time_t timestamp = ntp.get_timestamp();
        timestamp = timestamp + 3600 * 9; // Japan Time +9 hours offset
        if (timestamp < 0) {
            printf("An error occurred when getting the time. Code: %u\r\n", timestamp);
        } 
        else 
        {
            printf("Current time is %s\r\n", ctime(&timestamp));
            if (!bSynClock)
            {
                bSynClock   = true;
                timeinfo    = localtime(&timestamp);
                day         = (unsigned char) timeinfo -> tm_mday;
                mth         = (unsigned char) (timeinfo -> tm_mon + 1);
                year        = (unsigned char) (timeinfo -> tm_year + 1900 - 2000);
                dow         = (unsigned char) (timeinfo -> tm_wday + 1);
                hr          = (unsigned char) timeinfo -> tm_hour;
                min         = (unsigned char) timeinfo -> tm_min;
                sec         = (unsigned char) timeinfo -> tm_sec;
                clk.set_datetime(day, mth, year, dow, hr, min, sec);
                printf("RTC module date and time: %s %d/%d/%d  %d:%d:%d synchronized with NTP sertver.\r\n", swd[dow], day, mth, year, hr, min, sec);
            }
            else 
            {
                clk.get_date(day, mth, year, dow);
                clk.get_time(hr, min, sec);
                printf("RTC module date and time: %s %d/%d/%d  %d:%d:%d \r\n", swd[dow], day, mth, year, hr, min, sec);
            }
        }
        
        printf("Waiting 10 seconds before trying again...\r\n");
       // wait(10.0);  
       ThisThread::sleep_for(10s);      
    }
}

