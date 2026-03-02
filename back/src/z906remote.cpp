#include "z906remote.h"
#include "endpoints.h"
#include "environment.h"
#include "version.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <time.h>

namespace z906remote {
    void handle_current_effect(JsonWriter &);
    void handle_decode_mode_state(JsonWriter &);
    void handle_get_firmware(JsonWriter &);
    void handle_get_idle_time(JsonWriter &);
    void handle_get_status(JsonWriter &);
    void handle_get_temperature(JsonWriter &);
    void handle_get_volume(JsonWriter &);
    void handle_muted_state(JsonWriter &);
    void on_connected(const WiFiEventStationModeConnected &);
    void onWebSocketMessage(void *, uint8_t *, size_t);
    int  respond_to_request(AsyncWebServerRequest *, const Endpoint &);

    static AsyncWebServer      SERVER(80);
    static AsyncEventSource    EVENTS("/events");
    static AsyncCorsMiddleware CORS;
    const uint32_t             timerDelay = 60000;
    time_t                     currentTime;
    int8_t                     lastExecutedDay = -1;
    char                       JSON_BUFFER[512];
    char                       FIRMWARE_LATEST[16]  = FIRMWARE_VERSION;
    constexpr char             GITHUB_API[]         = "api.github.com";
    bool                       queueBroadcastStatus = false;
#ifndef COMPILE_UNIX_TIME
#    define COMPILE_UNIX_TIME 1767268800
#endif
    constexpr time_t COMPILE_TIME = COMPILE_UNIX_TIME;

    // Instantiate a Z906 object and attach to Serial
    Z906 LOGI(Serial);

    /**
     * Setup and connect to a WiFi network.
     */
    void init_wifi() {
        // Setup station mode.
        WiFi.mode(WIFI_STA);

        // Stored WiFi credentials.
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        WiFi.onStationModeConnected(on_connected);
    }

    /**
     * Setup the NTP client and update time from NTP Server
     */
    void start_NTP() {
        unsigned long start = millis();

        configTime(0, 0, "pool.ntp.org");
        while ((currentTime = time(nullptr)) < 100000) {
            if (millis() - start >= 5000) {
                currentTime = COMPILE_TIME;
                break;
            }
            delay(200);
        }
    }

    /**
     * Update time from NTP Server
     */
    void update_NTP() {
        static bool timeReady = false;
        time_t      now       = time(nullptr);

        if (!timeReady) {
            if (now > 100000) {
                timeReady = true;
            }
        } else {
            currentTime = now;
        }
    }

    /**
     * Connect to the WiFi.
     */
    void connect_to_wifi() {
        // Wait for connection.
        while (WiFi.begin() != WL_CONNECTED) {
            delay(1000);
        }
    }

    /**
     * Checks if the ESP is connected to WiFi.
     * @return true if the WiFi is connected (WL_CONNECTED), false otherwise.
     */
    bool isConnected() { return (WiFi.status() == WL_CONNECTED); }

    /**
     * Fetch the latest GitHub tag and update FIRMWARE_LATEST.
     */
    void getLatestTag() {
        WiFiClientSecure client;
        char             latestTag[16];
        size_t           len;

        client.setInsecure();
        client.setBufferSizes(256, 256);
        client.setTimeout(5000);
        if (!client.connect(GITHUB_API, 443))
            return;

        client.println("GET /repos/nforay/Z906Remote/tags?per_page=1 HTTP/1.1");
        client.print("Host: ");
        client.println(GITHUB_API);
        client.println("User-Agent: Esp8266");
        client.println("Connection: close");
        client.println();

        if (!client.find("\r\n\r\n") || !client.find("\"name\"") || !client.find("\"")) {
            return;
        }

        len = client.readBytesUntil('"', latestTag, sizeof(latestTag) - 1);
        if (len == 0)
            return;

        latestTag[len] = '\0';
        if (strncmp(latestTag, FIRMWARE_LATEST, sizeof(FIRMWARE_LATEST)) != 0) {
            strncpy(FIRMWARE_LATEST, latestTag, sizeof(FIRMWARE_LATEST) - 1);
            FIRMWARE_LATEST[sizeof(FIRMWARE_LATEST) - 1] = '\0';
            broadcastNotification("Update Available", ToastType::info);
        }
        EVENTS.send(latestTag, "update", millis());
    }

    /**
     * Trigger a daily check for the latest GitHub tag.
     */
    void checkForUpdate() {
        time_t     epoch = currentTime;
        struct tm *ptm   = gmtime(&epoch);
        int        today = ptm->tm_yday;

        if (today != lastExecutedDay) {
            lastExecutedDay = static_cast<int8_t>(today);
            getLatestTag();
        }
    }

    /**
     * When connection is restored, log the details.
     */
    void on_connected(const WiFiEventStationModeConnected &) {
        // Set the hostname,
        WiFi.hostname(HOSTNAME);

        // Configure MDNS.
        MDNS.begin(HOSTNAME);
    }

    /**
     * Log a message to all SSE clients
     */
    void broadcastLog(const char *message, LogLevel level) {
        JsonWriter jw;

        if (!EVENTS.count())
            return;

        jw.init(JSON_BUFFER, sizeof(JSON_BUFFER));
        jw.lit("{\"level\":");
        jw.quote(toString(level));
        jw.comma();
        jw.lit("\"msg\":");
        jw.quote(message);
        jw.lit("}");
        jw.buf[jw.len] = 0;

        EVENTS.send(JSON_BUFFER, "log", millis());
    }

    /**
     * Send a notification to all SSE clients
     */
    void broadcastNotification(const char *message, ToastType type) {
        JsonWriter jw;

        if (!EVENTS.count())
            return;
        jw.init(JSON_BUFFER, sizeof(JSON_BUFFER));
        jw.lit("{\"type\":");
        jw.quote(toString(type));
        jw.comma();
        jw.lit("\"text\":");
        jw.quote(message);
        jw.lit("}");
        jw.buf[jw.len] = 0;

        EVENTS.send(JSON_BUFFER, "notification", millis());
    }

    /**
     * Send the status content as JSON to all SSE clients
     */
    void broadcastPartialStatus(const char *serializedJson) {
        if (!EVENTS.count())
            return;
        EVENTS.send(serializedJson, "status", millis());
    }

    /**
     * Send the status content as JSON to all SSE clients
     */
    void broadcastStatus() {
        JsonWriter jw;

        if (!EVENTS.count())
            return;
        // workaround until proper cmd response response handling is implemented
        /*         if (!LOGI.request(GET_STATUS))
                    return; */
        LOGI.request(GET_STATUS);
        jw.init(JSON_BUFFER, sizeof(JSON_BUFFER));
        jw.lit("{\"status\":{");
        handle_get_status(jw);
        jw.lit("}}");
        jw.buf[jw.len] = 0;
        EVENTS.send(JSON_BUFFER, "status", millis());
    }

    void loop() { MDNS.update(); }

    /**
     * Run broadcastStatus() periodically
     */
    void updateClients() {
        static uint32_t lastPing   = 0;
        static uint32_t lastUpdate = 0;
        uint32_t        now        = millis();

        if (queueBroadcastStatus || (now - lastUpdate > timerDelay)) {
            queueBroadcastStatus = false;
            lastUpdate           = now;
            broadcastStatus();
            checkForUpdate();
        }
        if (now - lastPing > 15000) {
            lastPing = now;
            EVENTS.send("ping", "heartbeat");
        }
    }

    /**
     * Setup the web server.
     */
    void init_web_server() {
        CORS.setOrigin("*");
        CORS.setMethods("GET, OPTIONS");

        SERVER.addMiddleware(&CORS);

        SERVER.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse(LittleFS, "/index.html", "text/html");
            response->addHeader("Cache-Control",
                                "public, max-age=31536000, immutable");
            request->send(response);
        });

        SERVER.serveStatic("/", LittleFS, "/")
            .setCacheControl("max-age=1209600")
            .setLastModified(currentTime)
            .setTryGzipFirst(true);

        SERVER.addHandler(&EVENTS).addMiddleware(
            [](AsyncWebServerRequest *request, ArMiddlewareNext next) {
                if (EVENTS.count() > 5) {
                    request->send(503, "text/plain", "Server is busy");
                } else {
                    // process next middleware and at the end the handler
                    next();
                }
            });

        EVENTS.onConnect([](AsyncEventSourceClient *) { broadcastStatus(); });

        for (const Endpoint &e : endpoints) {
            SERVER.on(e.path, HTTP_GET, [e](AsyncWebServerRequest *request) {
                AsyncWebServerResponse *response =
                    request->beginResponse(respond_to_request(request, e),
                                           "application/json", JSON_BUFFER);
                response->addHeader("Access-Control-Allow-Origin", "*");
                request->send(response);
            });
        }

        SERVER.begin();
    }

    /**
     * Respond to a HTTP request for the given endpoint.
     */
    int respond_to_request(AsyncWebServerRequest *request, const Endpoint &endpoint) {
        JsonWriter jw;
        uint8_t    parsedValue = 0;
        int        cmdResponse;
        int        code  = 200;
        long       value = 0;

        jw.init(JSON_BUFFER, sizeof(JSON_BUFFER));
        if (LOGI.request(VERSION) == 0) {
            jw.lit("{\"status\":\"disconnected\"}");
            jw.buf[jw.len] = 0;
            return code;
        }
        jw.lit("{\"status\":\"connected\"");
#ifdef DEBUG_BUILD
        jw.lit(",\"debug\":{");
        jw.lit("\"heap\":");
        jw.writeNumber(ESP.getFreeHeap());
        jw.lit(",\"max_block\":");
        jw.writeNumber(ESP.getMaxFreeBlockSize());
        jw.lit(",\"frag\":");
        jw.writeNumber(ESP.getHeapFragmentation());
        jw.lit(",\"free_stack\":");
        jw.writeNumber(ESP.getFreeContStack());
        jw.lit(",\"reset\":");
        jw.quote(ESP.getResetReason().c_str());
        jw.lit(",\"path\":");
        jw.quote(endpoint.path);
        jw.lit(",\"type\":");
        jw.writeNumber(endpoint.type);
        jw.lit(",\"action\":");
        jw.writeNumber(endpoint.action);
        jw.lit(",\"ip\":");
        jw.quote(WiFi.localIP().toString().c_str());
        jw.lit(",\"rssi\":");
        jw.writeNumber(WiFi.RSSI());
        jw.lit(",\"uptime\":");
        jw.writeNumber(millis() / 1000);
        if (request->hasParam("value")) {
            jw.lit(",\"value\":");
            jw.quote(request->getParam("value")->value().c_str());
            jw.lit(",\"valueInt\":");
            jw.writeNumber(value = request->getParam("value")->value().toInt());
        }
        jw.lit("}");
#endif

        switch (endpoint.type) {
        case EndpointType::SelectInput:
            LOGI.input(endpoint.action, LOGI.input_effect(endpoint.action));
            queueBroadcastStatus = true;
            break;
        case EndpointType::RunCommand:
            cmdResponse = LOGI.cmd(endpoint.action);
            if (cmdResponse) {
                queueBroadcastStatus = true;
                jw.lit(",\"success\":true,\"value\":");
                jw.writeNumber(cmdResponse);
            } else {
                jw.lit(",\"success\":false");
            }
            break;
        case EndpointType::SetValue:
            value = request->getParam("value")->value().toInt();
            if (validate_input_value(value, parsedValue)) {
                LOGI.cmd(endpoint.action, parsedValue);
                queueBroadcastStatus = true;
                jw.lit(",\"success\":true");
            } else {
                code = 400;
                jw.lit(",\"success\":false,\"message\":\"Invalid value. Value "
                       "must be between 0 and 255.\"");
            }
            break;
        case EndpointType::GetValue:
            jw.lit(",\"success\":true,\"value\":");
            jw.writeNumber(LOGI.request(endpoint.action));
            break;
        case EndpointType::RunFunction:
            switch (endpoint.action) {
            case FunctionAction::Status: {
                jw.lit(",\"data\":{");
                handle_get_status(jw);
                jw.lit("}");
                break;
            }
            case FunctionAction::Mute:
                handle_muted_state(jw);
                break;
            case FunctionAction::Effect:
                handle_current_effect(jw);
                break;
            case FunctionAction::Temperature:
                handle_get_temperature(jw);
                break;
            case FunctionAction::Decode:
                handle_decode_mode_state(jw);
                break;
            case FunctionAction::Volume:
                handle_get_volume(jw);
                break;
            case FunctionAction::Idle:
                handle_get_idle_time(jw);
                break;
            case FunctionAction::Firmware:
                handle_get_firmware(jw);
            default: // do nothing
                break;
            }
            break;
            // fall through
        default:
            code = 405;
            jw.lit(",\"success\":false,\"message\":\"Your action was "
                   "recognised, but it is not supported.\"");
            break;
        }
        jw.lit("}");
        jw.buf[jw.len] = 0;

        return code;
    }

    inline void handle_get_status(JsonWriter &jw) {
        const Z906::t_packetdata packet = LOGI.get_data();

        jw.lit("\"main_level\":");
        jw.writeNumber(packet.main_level);
        jw.lit(",\"center_level\":");
        jw.writeNumber(packet.center_level);
        jw.lit(",\"rear_level\":");
        jw.writeNumber(packet.rear_level);
        jw.lit(",\"sub_level\":");
        jw.writeNumber(packet.sub_level);
        jw.lit(",\"current_input\":");
        jw.writeNumber(packet.current_input);
        jw.lit(",\"current_fx\":");
        jw.writeNumber(LOGI.current_effect());
        jw.lit(",\"muted\":");
        jw.writeBool(LOGI.muted_state());
        jw.lit(",\"decode_mode\":");
        jw.writeBool(LOGI.decode_mode());
        jw.lit(",\"fx_input_1\":");
        jw.writeNumber(packet.fx_input_1);
        jw.lit(",\"fx_input_2\":");
        jw.writeNumber(packet.fx_input_2);
        jw.lit(",\"fx_input_3\":");
        jw.writeNumber(packet.fx_input_3);
        jw.lit(",\"fx_input_4\":");
        jw.writeNumber(packet.fx_input_4);
        jw.lit(",\"fx_input_5\":");
        jw.writeNumber(packet.fx_input_5);
        jw.lit(",\"fx_input_aux\":");
        jw.writeNumber(packet.fx_input_aux);
        jw.lit(",\"spdif_status\":");
        jw.writeNumber(packet.spdif_status);
        jw.lit(",\"signal_status\":");
        jw.writeNumber(packet.signal_status);
        jw.lit(",\"stby\":");
        jw.writeNumber(packet.stby);
        jw.lit(",\"auto_stby\":");
        jw.writeNumber(packet.auto_stby);
    }

    /**
     * Get the muted state.
     */
    inline void handle_muted_state(JsonWriter &jw) {
        jw.lit(",\"success\":true,\"value\":");
        jw.writeBool(LOGI.muted_state());
    }

    /**
     * Get the Effect on the current input
     */
    inline void handle_current_effect(JsonWriter &jw) {
        jw.lit(",\"success\":true,\"value\":");
        jw.writeNumber(LOGI.current_effect());
    }

    /**
     * Handle the getTemperature function.
     */
    inline void handle_get_temperature(JsonWriter &jw) {
        const uint8_t value = LOGI.main_sensor();

        jw.lit(",\"success\":");
        jw.writeBool(!value ? false : true);
        jw.lit(",\"value\":");
        jw.writeNumber(value);
    }

    /**
     * Get the 5.1 Decode Mode state.
     */
    inline void handle_decode_mode_state(JsonWriter &jw) {
        jw.lit(",\"success\":true,\"value\":");
        jw.writeBool(LOGI.decode_mode());
    }

    /**
     * Get the volume on the current input
     */
    inline void handle_get_volume(JsonWriter &jw) {
        jw.lit(",\"success\":true,\"value\":");
        jw.writeNumber(LOGI.input_volume());
    }

    /**
     * Get the amplifier idle time
     */
    inline void handle_get_idle_time(JsonWriter &jw) {
        jw.lit(",\"success\":true,\"value\":");
        jw.writeNumber(LOGI.idle_time());
    }

    /**
     * Get the ESP8266 firmware version
     */
    inline void handle_get_firmware(JsonWriter &jw) {
        jw.lit(",\"success\":true,\"version\":{\"current\":");
        jw.flash(FIRMWARE_VERSION);
        jw.lit(",\"latest\":");
        jw.quote(FIRMWARE_LATEST);
        jw.lit(",\"compiled\":");
        jw.writeNumber(COMPILE_TIME);
        jw.lit("}");
    }

    /**
     * Validate and parse the input value.
     * @param value The input value to validate.
     * @param result Reference to a uint8_t variable where the parsed value
     *               will be stored if validation succeeds.
     * @return true if the value is valid, false otherwise.
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
