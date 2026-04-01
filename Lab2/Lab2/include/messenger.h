#include <Arduino.h>
#include "WiFiS3.h"

#define EMAIL_RECIPIENT   "tpurcell@uiowa.edu"
#define EMAIL_SENDER      "tylerp0417@gmail.com"
#define EMAIL_SENDER_NAME "Tyler Purcell"

class messenger {
  public:
    messenger();
    bool sendMessage(int hour, int minute, int month, int day, int year);

  private:
    bool sendEmail(const String& subject, const String& body);
    static String formatMessage(int hour, int minute, int month, int day, int year);
    static String twoDigits(int value);
};
