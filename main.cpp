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
    clk.init();
    unsigned char day;
    unsigned char mth;
    unsigned char year;
    unsigned char dow;
    unsigned char hr;
    unsigned char min;
    unsigned char sec;
    clk.get_date(day, mth, year, dow);
    clk.get_time(hr, min, sec);

    printf("RTC module date %d / %d / %d (%d), time %d : %d : %d \r\n", day, mth, year, dow, hr, min, sec);
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
        }
        
        printf("Waiting 10 seconds before trying again...\r\n");
       // wait(10.0);  
       ThisThread::sleep_for(10s);      
    }
}
