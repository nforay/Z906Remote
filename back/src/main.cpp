/**
 * Internet Connected Logitech Z906 API.
 * Based on the works of @Zarpli and @LewisSmallwood.
 * (https://github.com/zarpli/LOGItech-Z906/)
 * (https://github.com/LewisSmallwood/IoT-Logitech-Z906)
 */
#include "endpoints.h"
#include "environment.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <WString.h>
#include <WiFiUdp.h>
#include <Z906.h>


#define CONTENT_TEXT "text/plain"
#define CONTENT_JSON "application/json"

namespace z906remote {

    void init_wifi();
    void connect_to_wifi();
    void on_connected();
    void init_web_server();
    void handle_request();
    void respond_to_request(const Endpoint &endpoint);
    void handle_get_status();
    void handle_muted_state();
    void handle_get_temperature();
    void handle_decode_mode_state();
    void handle_current_effect();
    bool validate_input_value(const String &argName, uint8_t &result);

    ESP8266WebServer SERVER(80);
    ESP8266WiFiMulti WIFIMULTI;

    // Instantiate a Z906 object and attach to Serial
    Z906 LOGI(Serial);

    /**
     * Setup and connect to a WiFi network.
     */
    void init_wifi() {
        // Setup station mode.
        WiFi.mode(WIFI_STA);

        // Stored WiFi credentials.
        for (const Network &network : network_credentials)
            WIFIMULTI.addAP(network.ssid, network.password);
        delay(2000);

        // Connect to the WiFi.
        connect_to_wifi();
    }

    /**
     * Connect to the WiFi.
     */
    void connect_to_wifi() {
        // Wait for connection.
        while (WIFIMULTI.run() != WL_CONNECTED) {
            delay(1000);
        }

        on_connected();
    }

    /**
     * When connection is restored, log the details.
     */
    void on_connected() {
        // Set the hostname,
        WiFi.hostname("LOGITECH-Z906");

        // Configure MDNS.
        MDNS.begin("logitech-z906");
    }

    /**
     * Setup the web server.
     */
    void init_web_server() {
        SERVER.onNotFound(handle_request);
        SERVER.enableCORS(true);
        SERVER.begin();
    }

    /**
     * Handle a HTTP request.
     */
    void handle_request() {
        String endpoint = SERVER.uri();

        // CORS preflight
        if (SERVER.method() == HTTP_OPTIONS) {
            SERVER.sendHeader("Access-Control-Allow-Methods", "GET");
            SERVER.sendHeader("Access-Control-Allow-Headers",
                              "access-control-allow-origin");
            return SERVER.send(200, CONTENT_TEXT);
        }

        if (endpoint == "/" || endpoint == "/index.html") {
            fs::File file = LittleFS.open("/index.html", "r");
            SERVER.sendHeader("cache-control", "no-cache");
            return SERVER.send(200, "text/html", file, file.size());
        } else if (endpoint == "/assets/Default.css") {
            fs::File file = LittleFS.open("/assets/Default.css", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "text/css", file, file.size());
        } else if (endpoint == "/assets/Home.css") {
            fs::File file = LittleFS.open("/assets/Home.css", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "text/css", file, file.size());
        } else if (endpoint == "/assets/index.css") {
            fs::File file = LittleFS.open("/assets/index.css", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "text/css", file, file.size());
        } else if (endpoint == "/assets/ssrBoot.css") {
            fs::File file = LittleFS.open("/assets/ssrBoot.css", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "text/css", file, file.size());
        } else if (endpoint == "/assets/Default.js") {
            fs::File file = LittleFS.open("/assets/Default.js", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "application/javascript", file, file.size());
        } else if (endpoint == "/assets/Home.js") {
            fs::File file = LittleFS.open("/assets/Home.js", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "application/javascript", file, file.size());
        } else if (endpoint == "/assets/index.js") {
            fs::File file = LittleFS.open("/assets/index.js", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "application/javascript", file, file.size());
        } else if (endpoint == "/assets/ssrBoot.js") {
            fs::File file = LittleFS.open("/assets/ssrBoot.js", "r");
            SERVER.sendHeader("content-encoding", "gzip");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "application/javascript", file, file.size());
        } else if (endpoint == "/favicon.ico") {
            fs::File file = LittleFS.open("/favicon.ico", "r");
            SERVER.sendHeader("cache-control", "max-age=31536000");
            return SERVER.send(200, "image/x-icon", file, file.size());
        }

        if (endpoint.length() > 1)
            endpoint = endpoint.substring(1);

        // Check if the Z906 is connected before processing any commands.
        if (LOGI.request(VERSION) == 0)
            return SERVER.send(200, CONTENT_JSON, R"({"status": "disconnected"})");

        // Handle defined endpoints.
        for (const Endpoint &e : endpoints) {
            if (e.path == endpoint)
                return respond_to_request(e);
        }

        // If not found, redirect to home.
        SERVER.sendHeader("Location", String("/"), true);
        SERVER.send(302, CONTENT_TEXT, "");
    }

    /**
     * Respond to a HTTP request for the given endpoint.
     */
    void respond_to_request(const Endpoint &endpoint) {
        JsonDocument doc;
        String       output;
        int          code = 200;
        uint8_t      parsedValue;

        doc["status"]  = "connected";
        doc["success"] = true;

        switch (endpoint.type) {
        case EndpointType::SelectInput:
            LOGI.input(endpoint.action);
            break;
        case EndpointType::RunCommand:
            LOGI.cmd(endpoint.action);
            break;
        case EndpointType::SetValue:
            if (validate_input_value("value", parsedValue)) {
                LOGI.cmd(endpoint.action, parsedValue);
            } else {
                code = 400;

                doc["success"] = false;
                doc["message"] =
                    "Invalid value. Value must be between 0 and 255.";
            }
            break;
        case EndpointType::GetValue:
            doc["value"] = LOGI.request(endpoint.action);
            break;
        case EndpointType::RunFunction:
            switch (endpoint.action) {
            case FunctionAction::Status:
                return handle_get_status();
            case FunctionAction::Mute:
                return handle_muted_state();
            case FunctionAction::Effect:
                return handle_current_effect();
            case FunctionAction::Temperature:
                return handle_get_temperature();
            case FunctionAction::Decode:
                return handle_decode_mode_state();
            default: // do nothing
                break;
            }
            // fall through
        default:
            doc["success"] = false;
            code           = 405;
            doc["message"] =
                "Your action was recognised, but it is not supported.";
            break;
        }
        serializeJson(doc, output);
        return SERVER.send(code, CONTENT_JSON, output);
    }

    void handle_get_status() {
        const Z906::t_packetdata packet = LOGI.get_data();
        JsonDocument             doc;
        JsonObject               data = doc["data"].to<JsonObject>();
        String                   output;

        doc["status"]         = "connected";
        doc["success"]        = true;
        data["main_level"]    = packet.main_level;
        data["center_level"]  = packet.center_level;
        data["rear_level"]    = packet.rear_level;
        data["sub_level"]     = packet.sub_level;
        data["current_input"] = packet.current_input;
        data["current_fx"]    = LOGI.current_effect();
        data["muted"]         = LOGI.muted_state();
        data["decode_mode"]   = LOGI.decode_mode();
        data["fx_input_1"]    = packet.fx_input_1;
        data["fx_input_2"]    = packet.fx_input_2;
        data["fx_input_3"]    = packet.fx_input_3;
        data["fx_input_4"]    = packet.fx_input_4;
        data["fx_input_5"]    = packet.fx_input_5;
        data["fx_input_aux"]  = packet.fx_input_aux;
        data["spdif_status"]  = packet.spdif_status;
        data["signal_status"] = packet.signal_status;
        data["stby"]          = packet.stby;
        data["auto_stby"]     = packet.auto_stby;
        serializeJson(doc, output);

        return SERVER.send(200, CONTENT_JSON, output);
    }

    /**
     * Get the muted state.
     */
    void handle_muted_state() {
        JsonDocument doc;
        String       output;

        doc["status"]  = "connected";
        doc["success"] = true;
        doc["value"]   = LOGI.muted_state();
        serializeJson(doc, output);

        return SERVER.send(200, CONTENT_JSON, output);
    }

    /**
     * Get the Effect on the current input
     */
    void handle_current_effect() {
        JsonDocument doc;
        String       output;

        doc["status"]  = "connected";
        doc["success"] = true;
        doc["value"]   = LOGI.current_effect();
        serializeJson(doc, output);

        return SERVER.send(200, CONTENT_JSON, output);
    }

    /**
     * Handle the getTemperature function.
     */
    void handle_get_temperature() {
        JsonDocument doc;
        String       output;

        const uint8_t value = LOGI.main_sensor();
        doc["status"]       = "connected";
        doc["success"]      = true;
        doc["value"]        = value;
        serializeJson(doc, output);
        if (value > 0)
            return SERVER.send(200, CONTENT_JSON, output);

        doc["success"] = false;
        return SERVER.send(200, CONTENT_JSON, output);
    }

    /**
     * Get the 5.1 Decode Mode state.
     */
    void handle_decode_mode_state() {
        JsonDocument doc;
        String       output;

        doc["status"]  = "connected";
        doc["success"] = true;
        doc["value"]   = LOGI.decode_mode();
        serializeJson(doc, output);

        return SERVER.send(200, CONTENT_JSON, output);
    }

    /**
     * Validate and parse the input value.
     * Returns true if the value is valid, false otherwise.
     * If valid, the parsed value is stored in the 'result' parameter.
     */
    bool validate_input_value(const String &argName, uint8_t &result) {
        if (SERVER.hasArg(argName)) {
            String        valueStr = SERVER.arg(argName);
            const int32_t value    = valueStr.toInt();

            // Check if the conversion was successful
            if (value != 0L || valueStr.equals("0")) {

                if (value >= 0L && value <= 255L) {
                    // Valid value, store the result.
                    result = static_cast<uint8_t>(value);
                    return true; // Value is valid.
                } else {
                    // Value is not in the valid range.
                    return false;
                }
            } else {
                // Invalid value format.
                return false;
            }
        } else {
            // No value provided in the request.
            return false;
        }
    }

} // namespace z906remote

/**
 * Setup
 */
void setup() {
    LittleFS.begin();
    z906remote::init_wifi();
    z906remote::init_web_server();
    ArduinoOTA.setPassword(OTApassword);
    ArduinoOTA.begin();
}

/**
 * Loop
 */
void loop() {
    if (WiFi.status() != WL_CONNECTED)
        z906remote::connect_to_wifi();
    z906remote::SERVER.handleClient();
    ArduinoOTA.handle();
}
