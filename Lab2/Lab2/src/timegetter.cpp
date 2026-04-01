#include <WiFiUdp.h>
#include <RTC.h>
#include "timegetter.h"


// Send NTP request, wait for response, then writes the result into the hardware RTC so RTC.getTime() is accurate
bool timegetter::syncRTC() {
    WiFiUDP udp;
    udp.begin(NTP_PORT);

    Serial.print("[timegetter] Querying ");
    Serial.print(NTP_SERVER);
    Serial.print(" ... ");

    sendNTPPacket(udp, NTP_SERVER);

    // Wait 5 seconds for a response
    unsigned long start = millis();
    while (udp.parsePacket() == 0) {
        if (millis() - start > 5000) {
            Serial.println("timed out.");
            udp.stop();
            return false;
        }
        delay(100);
    }

    unsigned long epochUTC = parseNTPResponse(udp);
    udp.stop();

    if (epochUTC == 0) {
        Serial.println("invalid response.");
        return false;
    }

    long epochLocal = (long)epochUTC + UTC_OFFSET_SEC;

    unsigned long t = (unsigned long)epochLocal;

    int sec   =  t % 60; t /= 60;
    int min   =  t % 60; t /= 60;
    int hour  =  t % 24; t /= 24;


    // Using algorithm from http://howardhinnant.github.io/date_algorithms.html
    unsigned long z = t + 719468;
    unsigned long era = z / 146097;
    unsigned int doe = (unsigned int)(z - era * 146097);
    unsigned int yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    unsigned long y  = (unsigned long)yoe + era * 400;
    unsigned int doy = doe - (365*yoe + yoe/4 - yoe/100);
    unsigned int mp  = (5*doy + 2) / 153;
    unsigned int day = doy - (153*mp + 2)/5 + 1;
    unsigned int month = mp < 10 ? mp + 3 : mp - 9;
    y += (month <= 2);

    RTCTime rtcTime(
        (int)day,
        (Month)month,        
        (int)y,
        hour, min, sec,
        DayOfWeek::MONDAY,   // placeholder
        SaveLight::SAVING_TIME_INACTIVE
    );

    if (!RTC.setTime(rtcTime)) {
        Serial.println("RTC.setTime() failed.");
        return false;
    }

    Serial.print("RTC set to ");
    Serial.print(day);   Serial.print("/");
    Serial.print(month); Serial.print("/");
    Serial.print((int)y); Serial.print(" ");
    Serial.print(hour);  Serial.print(":");
    if (min < 10) Serial.print("0");
    Serial.print(min);   Serial.print(":");
    if (sec < 10) Serial.print("0");
    Serial.println(sec);

    return true;
}

// make & send 48-byte NTP request
void timegetter::sendNTPPacket(WiFiUDP& udp, const char* address) {
    byte packet[NTP_PACKET_SIZE] = {0};
    packet[0] = 0b11100011;  
    packet[1] = 0;           
    packet[2] = 6;           
    packet[3] = 0xEC;        
    packet[12] = 49;
    packet[13] = 0x4E;
    packet[14] = 49;
    packet[15] = 52;

    udp.beginPacket(address, NTP_PORT);
    udp.write(packet, NTP_PACKET_SIZE);
    udp.endPacket();
}

// read 48-byte reply & return epoch
unsigned long timegetter::parseNTPResponse(WiFiUDP& udp) {
    byte packet[NTP_PACKET_SIZE];
    udp.read(packet, NTP_PACKET_SIZE);


    unsigned long high = word(packet[40], packet[41]);
    unsigned long low  = word(packet[42], packet[43]);
    unsigned long ntpTime  = (high << 16) | low;

    // Convert NTP epoch
    if (ntpTime < 2208988800UL) return 0;
    return ntpTime - 2208988800UL;
}
