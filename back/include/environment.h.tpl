#pragma once

struct Network {
    const char *ssid;
    const char *password;
};

const Network network_credentials[] = {
    {"MyWiFiSSID2", "secret_password"},
    {"MyWiFiSSID2", "secret_password"},
};

const char *OTApassword = "MYPASSSWORDHERE";
