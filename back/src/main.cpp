/**
 * Internet Connected Logitech Z906 API.
 * Based on the works of @Zarpli and @LewisSmallwood.
 * (https://github.com/zarpli/LOGItech-Z906/)
 * (https://github.com/LewisSmallwood/IoT-Logitech-Z906)
 */
#include "environment.h"
#include "z906remote.h"
#include "z906remote_ir.h"
#include "z906remote_mqtt.h"

#ifdef INCLUDE_OTA
#    include <ArduinoOTA.h>
#endif
#include <LittleFS.h>

/**
 * Setup
 */
void setup() {
    LittleFS.begin();
    z906remote::infrared::enableIR();
    z906remote::init_wifi();
    z906remote::start_NTP();
    z906remote::init_web_server();
    z906remote::mqtt::init();
    z906remote::mqtt::connect();
#ifdef INCLUDE_OTA
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTApassword);
    ArduinoOTA.begin(true);
#endif
}

/**
 * Loop
 */
void loop() {
    z906remote::loop();
    if (!z906remote::isConnected()) {
        z906remote::connect_to_wifi();
    } else {
        if (!z906remote::mqtt::isConnected()) {
            z906remote::mqtt::connect();
        }
        z906remote::mqtt::loop();
        z906remote::infrared::processIR();
        z906remote::mqtt::updateStates();
        z906remote::update_NTP();
        z906remote::updateClients();
    }
#ifdef INCLUDE_OTA
    ArduinoOTA.handle();
#endif
}
