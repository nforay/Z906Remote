#pragma once
#include <Arduino.h>
#include <Z906.h>

namespace z906remote {
    enum class ToastType { info, success, warning, error };
    enum class LogLevel { log, info, warn, error };

    bool validate_input_value(long, uint8_t &);
    bool isConnected();
    void broadcastNotification(const char *, ToastType);
    void broadcastLog(const char *, LogLevel);
    void broadcastPartialStatus(const char *);
    void broadcastStatus();
    void checkForUpdate();
    void connect_to_wifi();
    void init_web_server();
    void init_wifi();
    void loop();
    void start_NTP();
    void update_NTP();
    void updateClients();

    extern Z906 LOGI;
    extern char JSON_BUFFER[512];
    extern char FIRMWARE_LATEST[16];

    struct JsonWriter {
        char  *buf;
        size_t size;
        size_t len;

        void init(char *b, size_t s) {
            buf  = b;
            size = s;
            len  = 0;
        }

        // write known-size literal from flash
        template <size_t N> inline void lit(const char (&s)[N]) {
            memcpy_P(buf + len, s, N - 1);
            len += N - 1;
        }

        // write constexpr char[] from flash
        template <size_t N> inline void flash(const char (&s)[N]) {
            buf[len++] = '"';
            memcpy_P(buf + len, s, N - 1);
            len += N - 1;
            buf[len++] = '"';
        }

        // runtime string
        void str(const char *s) {
            size_t l = strlen(s);
            memcpy(buf + len, s, l);
            len += l;
        }

        // quote runtime string
        void quote(const char *s) {
            buf[len++] = '"';
            str(s);
            buf[len++] = '"';
        }

        void colon() { buf[len++] = ':'; }
        void comma() { buf[len++] = ','; }

        void raw(const char *p, size_t l) {
            memcpy(buf + len, p, l);
            len += l;
        }

        void raw_P(const char *p, size_t l) {
            buf[len++] = '"';
            memcpy_P(buf + len, p, l);
            len += l;
            buf[len++] = '"';
        }

        template <typename T> void writeNumber(T value) {
            char tmp[12];
            itoa(static_cast<int32_t>(value), tmp, 10);
            str(tmp);
        }

        void writeBool(bool b) {
            if (b)
                raw("true", 4);
            else
                raw("false", 5);
        }
    };

    inline const char *toString(ToastType type) {
        switch (type) {
        case ToastType::success:
            return "success";
        case ToastType::warning:
            return "warning";
        case ToastType::error:
            return "error";
        default:
            return "info";
        }
    }

    inline const char *toString(LogLevel level) {
        switch (level) {
        case LogLevel::info:
            return "info";
        case LogLevel::warn:
            return "warn";
        case LogLevel::error:
            return "error";
        default:
            return "log";
        }
    }
} // namespace z906remote
