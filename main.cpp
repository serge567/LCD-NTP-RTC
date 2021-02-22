#include "mbed.h"
#include "ds1302.h" // RTC module
#include "TextLCD.h" // LCD 1602 with PCF8574, I2C bus
#include <string>
#include "Json.h"
//WIFI
#include "ESP8266Interface.h"


//WIFI
ESP8266Interface wifi(D1, D0);
DigitalOut ES8266RST(D2);
 
DigitalIn userButton(USER_BUTTON);

//LCD 1602
I2C i2c_lcd(D14,D15); // SDA, SCL
// 1001110  - 0x4E
// 100111	- 0x27
TextLCD_I2C lcd(&i2c_lcd, 0x4E, TextLCD::LCD16x2); // I2C bus, PCF8574 Slaveaddress, LCD Type
Ticker SecTick;

// Time synchronization
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

//

const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

void scan_demo(WiFiInterface *wifi)
{
    WiFiAccessPoint *ap;

    printf("Scan:\r\n");

    int count = wifi->scan(NULL, 0);

    /* Limit number of network arbitrary to 15 */
    count = count < 15 ? count : 15;

    ap = new WiFiAccessPoint[count];

    count = wifi->scan(ap, count);
    for (int i = 0; i < count; i++) {
        printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n", ap[i].get_ssid(),
               sec2str(ap[i].get_security()), ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
               ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
    }
    printf("%d networks available.\r\n", count);

    delete[] ap;
}

time_t http_demo(NetworkInterface *net)
{
    // Open a socket on the network interface, and create a TCP connection to mbed.org
    TCPSocket socket;
    socket.open(net);

    SocketAddress a;
    net->gethostbyname("worldtimeapi.org", &a);
    a.set_port(80);
    socket.connect(a);
    // Send a simple http request
    char sbuffer[] = "GET /api/timezone/Asia/Tokyo HTTP/1.1\r\nHost: worldtimeapi.org\r\n\r\n";
    int scount = socket.send(sbuffer, sizeof sbuffer);
    // DEBUG
    // printf("sent %d [%.*s]\r\n", scount, strstr(sbuffer, "\r\n") - sbuffer, sbuffer);

    // Recieve a simple http response and print out the response line
    char rbuffer[1024];
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    // DEBUG
    // printf("recv %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n") - rbuffer, rbuffer);
    // printf("recv %s\r\n", rbuffer);
    string sJSON = rbuffer;

    // Close the socket to return its memory and bring down the network interface
    socket.close();

    size_t posb = sJSON.find("{");
    size_t pose = sJSON.find("}");
    sJSON = sJSON.substr(posb, pose - posb + 1);
    // DEBUG
    // printf("%s \r\n", sJSON.c_str());

    Json oJSON ( sJSON.c_str(), strlen ( sJSON.c_str() ) );

    if ( !oJSON.isValidJson () )
    {   
        printf("Invalid JSON: %s \r\n", sJSON.c_str());
        return 0;
    }
    if ( oJSON.type (0) != JSMN_OBJECT )
    {
        printf ( "Invalid JSON.  ROOT element is not Object: %s \r\n", sJSON.c_str() );
        return 0;
    }
    int unixtimeIndex = oJSON.findKeyIndexIn ( "unixtime", 0 );
    char unixtimeValue [ 32 ];    
    if ( unixtimeIndex == -1 )
    {
            printf ("%s \r\n"," unixtime JSON key does not exist." );
    }
    else
    {
        int unixtimeValueIndex = oJSON.findChildIndexOf ( unixtimeIndex, -1 );
        if ( unixtimeValueIndex > 0 )
        {
            const char * valueStart  = oJSON.tokenAddress ( unixtimeValueIndex );
            int          valueLength = oJSON.tokenLength ( unixtimeValueIndex );
            strncpy ( unixtimeValue, valueStart, valueLength );
            unixtimeValue [ valueLength ] = 0; // NULL-terminate the string
            // DEBUG
            // printf ( "unixtime: %s \r\n", unixtimeValue );
            time_t tmconv;
            sscanf(unixtimeValue, "%d", &tmconv);
            return (time_t) tmconv;
        }
    }
    return 0;
}

string StrLeadZero(unsigned char number)
{
    string snumber = to_string(number);
    if (snumber.length() == 1)
    { snumber = "0" + snumber; }
    return snumber;
}

void RTC_PrintDateTime()
{
    clk.get_date(day, mth, year, dow);
    clk.get_time(hr, minu, sec);
    printf("RTC module date and time: %s, %s %s, %d  %s:%s:%s \r\n", swd[dow], smth[mth], StrLeadZero(day).c_str(), year + 2000, StrLeadZero(hr).c_str(), StrLeadZero(minu).c_str(), StrLeadZero(sec).c_str());   
}

void RTC_Synch()
{
    //
    printf("%s", "Reseting ESP8266 WIFI. \r\n");
    ES8266RST = 0;
    ThisThread::sleep_for(3s);
    ES8266RST = 1;
    ThisThread::sleep_for(10s);

    SocketAddress a;

    printf("WiFi example\r\n\r\n");

   // DEBUG
  //  scan_demo(&wifi);

    printf("\r\nConnecting...\r\n");
    int ret = wifi.connect("UsagiMoomin", "usachumuchu", NSAPI_SECURITY_WPA_WPA2);
    if (ret == 0) {
        printf("Success\r\n\r\n");
        printf("MAC: %s\r\n", wifi.get_mac_address());
        wifi.get_ip_address(&a);
        printf("IP: %s\r\n", a.get_ip_address());
        wifi.get_netmask(&a);
        printf("Netmask: %s\r\n", a.get_ip_address());
        wifi.get_gateway(&a);
        printf("Gateway: %s\r\n", a.get_ip_address());
        printf("RSSI: %d\r\n\r\n", wifi.get_rssi());

        timestamp = http_demo(&wifi);

        wifi.disconnect();

        printf("\r\nDone\r\n");

     }
     else 
     {
        printf("\r\nConnection error\r\n");
        timestamp = 0;
     }
    
    //
    printf("\r\ntimestamp: %d\r\n", timestamp);
    timestamp = timestamp + 3600 * 9; // Japan Time +9 hours offset
    timeinfo    = localtime(&timestamp);
    year        = (unsigned char) (timeinfo -> tm_year + 1900 - 2000);

    if ((year < 20) || (year > 25)) // 2020 and 2025 years
    {
        printf("An error occurred when getting the time. %d year got, it must be more 2020 and less 2025 \r\n", year + 2000);
    } 
    else 
    {
        printf("Current time is %s\r\n", ctime(&timestamp)); 
        day         = (unsigned char) timeinfo -> tm_mday;
        mth         = (unsigned char) (timeinfo -> tm_mon + 1);
        dow         = (unsigned char) (timeinfo -> tm_wday + 1);
        hr          = (unsigned char) timeinfo -> tm_hour;
        minu        = (unsigned char) timeinfo -> tm_min;
        sec         = (unsigned char) timeinfo -> tm_sec;
        clk.set_datetime(day, mth, year, dow, hr, minu, sec);
        printf("RTC module date and time: %s, %s %d, %d  %d:%d:%d synchronized with NTP sertver.\r\n", swd[dow], smth[mth], day, year + 2000, hr, minu, sec);   
    }
    lcd.cls();
}

void LCDDisplayTime()
{
    clk.get_date(day, mth, year, dow);
    clk.get_time(hr, minu, sec);
    lcd.locate(0, 0);
    lcd.printf("%s %s %s, %d ", swd[dow], smth[mth], StrLeadZero(day).c_str(), year + 2000); 
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
        if (userButton && bOneSecTick) 
        {  
           lcd.cls();
           lcd.locate(0, 0);
           lcd.printf("%s", "USER BUTTON"); 
           lcd.locate(0, 1);
           lcd.printf("%s", "PRESSED!"); 
           ThisThread::sleep_for(3s);
           lcd.cls();
           RTC_Synch(); 
        } 
        if (bOneSecTick)
        {
            bOneSecTick = false;
            LCDDisplayTime();
        }
    }
}
