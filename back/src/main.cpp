/**
 * Internet Connected Logitech Z906 API.
 * Based on the works of @Zarpli and @LewisSmallwood.
 * (https://github.com/zarpli/LOGItech-Z906/)
 * (https://github.com/LewisSmallwood/IoT-Logitech-Z906)
 */
#include "endpoints.h"
#include "environment.h"
#include "version.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <WString.h>
#include <WiFiUdp.h>
#include <Z906.h>


namespace z906remote {

    void init_wifi();
    void connect_to_wifi();
    void on_connected();
    void onWebSocketMessage(void *, uint8_t *, size_t);
    void broadcastMessage(const String &);
    void broadcastStatus();
    void updateClients();
    void init_web_server();
    void respond_to_request(AsyncWebServerRequest *, AsyncResponseStream *, const Endpoint &);
    void handle_get_status(JsonDocument &);
    void handle_muted_state(JsonDocument &);
    void handle_get_temperature(JsonDocument &);
    void handle_decode_mode_state(JsonDocument &);
    void handle_current_effect(JsonDocument &);
    void handle_get_volume(JsonDocument &);
    bool validate_input_value(long, uint8_t &);

    AsyncWebServer   SERVER(80);
    ESP8266WiFiMulti WIFIMULTI;
    AsyncWebSocket   WS("/ws");

    WiFiUDP       ntpUDP;
    NTPClient     timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
    time_t        currentTime;
    unsigned long lastUpdate = 0;
    unsigned long timerDelay = 60000;

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
     * When a WebSocket message is recieved, echo the content to all clients
     */
    void onWebSocketMessage(void *arg, uint8_t *data, size_t len) {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            WS.textAll(String("Echo: ") + (char *)data);
        }
    }

    /**
     * Send a message to all Websocket clients
     */
    void broadcastMessage(const String &message) { WS.textAll(message); }

    /**
     * Send the status content as JSON to all clients
     */
    void broadcastStatus() {
        JsonDocument doc;
        String       status;

        // workaround until proper cmd response response handling is implemented
        LOGI.request(GET_STATUS);

        handle_get_status(doc);
        serializeJson(doc, status);
        WS.textAll(status);
    }

    /**
     * Run broadcastStatus() periodically
     */
    void updateClients() {
        if (millis() - lastUpdate > timerDelay) {
            lastUpdate = millis();
            z906remote::broadcastStatus();
        }
    }

    /**
     * Setup the web server.
     */
    void init_web_server() {
        SERVER.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/index.html", "text/html");
        });

        SERVER.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/index.html", "text/html");
        });

        SERVER.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/favicon.ico", "image/x-icon");
        });

        SERVER.serveStatic("/assets", LittleFS, "/assets/")
            .setCacheControl("max-age=31536000")
            .setLastModified(currentTime)
            .setTryGzipFirst(true);

        SERVER.onNotFound([](AsyncWebServerRequest *request) {
            if (request->method() == HTTP_OPTIONS) {
                AsyncWebServerResponse *response = request->beginResponse(200);
                response->addHeader("Access-Control-Allow-Origin", "*");
                response->addHeader("Access-Control-Allow-Methods",
                                    "GET, OPTIONS");
                response->addHeader("Access-Control-Allow-Headers",
                                    "access-control-allow-origin");
                request->send(response);
                return;
            }

            AsyncWebServerResponse *response =
                request->beginResponse(302, "text/plain", "");
            response->addHeader("Location", "/");
            request->send(response);
        });

        for (const Endpoint &e : endpoints) {
            SERVER.on(e.path, HTTP_GET, [e](AsyncWebServerRequest *request) {
                AsyncResponseStream *response =
                    request->beginResponseStream("application/json");
                respond_to_request(request, response, e);
                request->send(response);
            });
        }

        WS.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
            switch (type) {
            case WS_EVT_CONNECT:
            case WS_EVT_DISCONNECT:
            case WS_EVT_PING:
            case WS_EVT_PONG:
            case WS_EVT_ERROR:
                break;
            case WS_EVT_DATA:
                onWebSocketMessage(arg, data, len);
                break;
            default:
                break;
            }
            (void)server;
            (void)client;
        });

        SERVER.addHandler(&WS);
        SERVER.begin();
    }

    /**
     * Respond to a HTTP request for the given endpoint.
     */
    void respond_to_request(AsyncWebServerRequest *request,
                            AsyncResponseStream *response, const Endpoint &endpoint) {
        JsonDocument doc;
        uint8_t      parsedValue = 0;
        int          cmdResponse;
        int          code  = 200;
        long         value = 0;

        if (LOGI.request(VERSION) == 0) {
            response->print("{\"status\":\"disconnected\"}");
            response->setCode(code);
            return;
        }
        doc["status"]  = "connected";
        doc["success"] = true;
#ifdef DEBUG_BUILD
        JsonObject debug  = doc["debug"].to<JsonObject>();
        debug["version"]  = FIRMWARE_VERSION;
        debug["freeheap"] = ESP.getFreeHeap();
        debug["path"]     = endpoint.path;
        debug["type"]     = endpoint.type;
        debug["action"]   = endpoint.action;
#endif

        switch (endpoint.type) {
        case EndpointType::SelectInput:
            LOGI.input(endpoint.action);
            broadcastStatus();
            break;
        case EndpointType::RunCommand:
            cmdResponse = LOGI.cmd(endpoint.action);
            broadcastStatus();
            if (cmdResponse) {
                doc["value"] = cmdResponse;
            } else {
                doc["success"] = false;
            }
            break;
        case EndpointType::SetValue:
            value = request->getParam("value")->value().toInt();
#ifdef DEBUG_BUILD
            debug["value"]    = request->getParam("value")->value();
            debug["valueInt"] = value;
#endif
            if (validate_input_value(value, parsedValue)) {
                LOGI.cmd(endpoint.action, parsedValue);
                broadcastStatus();
            } else {
                code           = 400;
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
                handle_get_status(doc);
                break;
            case FunctionAction::Mute:
                handle_muted_state(doc);
                break;
            case FunctionAction::Effect:
                handle_current_effect(doc);
                break;
            case FunctionAction::Temperature:
                handle_get_temperature(doc);
                break;
            case FunctionAction::Decode:
                handle_decode_mode_state(doc);
                break;
            case FunctionAction::Volume:
                handle_get_volume(doc);
                break;
            default: // do nothing
                break;
            }
            break;
            // fall through
        default:
            code           = 405;
            doc["success"] = false;
            doc["message"] =
                "Your action was recognised, but it is not supported.";
            break;
        }
        serializeJson(doc, *response);
        response->setCode(code);
    }

    inline void handle_get_status(JsonDocument &doc) {
        const Z906::t_packetdata packet = LOGI.get_data();
        JsonObject               data   = doc["data"].to<JsonObject>();

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
    }

    /**
     * Get the muted state.
     */
    inline void handle_muted_state(JsonDocument &doc) {
        doc["value"] = LOGI.muted_state();
    }

    /**
     * Get the Effect on the current input
     */
    inline void handle_current_effect(JsonDocument &doc) {
        doc["value"] = LOGI.current_effect();
    }

    /**
     * Handle the getTemperature function.
     */
    inline void handle_get_temperature(JsonDocument &doc) {
        const uint8_t value = LOGI.main_sensor();
        doc["success"]      = !value ? false : true;
        doc["value"]        = value;
    }

    /**
     * Get the 5.1 Decode Mode state.
     */
    inline void handle_decode_mode_state(JsonDocument &doc) {
        doc["value"] = LOGI.decode_mode();
    }

    /**
     * Get the volume on the current input
     */
    inline void handle_get_volume(JsonDocument &doc) {
        doc["value"] = LOGI.input_volume();
    }

    /**
     * Validate and parse the input value.
     * Returns true if the value is valid, false otherwise.
     * If valid, the parsed value is stored in the 'result' parameter.
     */
    bool validate_input_value(const long value, uint8_t &result) {
        if (value >= 0L && value <= 255L) {
            // Valid value, store the result.
            result = static_cast<uint8_t>(value);
            return true; // Value is valid.
        } else {
            // Value is not in the valid range.
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
    z906remote::timeClient.begin();
    while (!z906remote::timeClient.update()) {
        z906remote::timeClient.forceUpdate();
    }
    z906remote::currentTime = z906remote::timeClient.getEpochTime();
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
    z906remote::timeClient.update();
    ArduinoOTA.handle();
    z906remote::updateClients();
    z906remote::WS.cleanupClients();
}
