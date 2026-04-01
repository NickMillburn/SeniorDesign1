#include <Arduino.h>
#include <WiFiUdp.h>
#include <RTC.h>

#define NTP_SERVER      "pool.ntp.org"
#define NTP_PORT        123
#define UTC_OFFSET_SEC  -18000   // CST time

class timegetter {
public:
    static bool syncRTC();

private:
    static const int NTP_PACKET_SIZE = 48;

    static void sendNTPPacket(WiFiUDP& udp, const char* address);
    static unsigned long parseNTPResponse(WiFiUDP& udp);
};
