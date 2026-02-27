#pragma once
#include <Arduino.h>

inline constexpr char WIFI_SSID[] = ENV_WIFI_SSID;
inline constexpr char WIFI_PASS[] = ENV_WIFI_PASS;

inline constexpr char HOSTNAME[] = ENV_HOSTNAME;

#ifdef INCLUDE_OTA
inline constexpr char OTApassword[] = ENV_OTAPASSWORD;
#endif

#ifdef INCLUDE_MQTT
inline constexpr char     MQTT_HOST[] = ENV_MQTT_HOST;
inline constexpr uint16_t MQTT_PORT   = ENV_MQTT_PORT;
inline constexpr char     MQTT_UID[]  = ENV_MQTT_UID;
inline constexpr char     MQTT_USER[] = ENV_MQTT_USER;
inline constexpr char     MQTT_PASS[] = ENV_MQTT_PASS;
#endif
