#include <Arduino.h>
#include "WiFiS3.h"
#include "messageConfig.h"

class messager{
    public:
    messager();

    void messager::checkNotify(int sensorIndex, float tempC, const messageConfig& cfg);

    private:
    bool sendEmail(const String& recipient, const String& subject, const String& body);

    // Expand {sensor} and {temp} tokens in the body template
    static String changeTemplate(const String& tmpl, int sensorIndex, float tempC);
}