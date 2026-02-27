#include "z906remote_mqtt.h"
#ifdef INCLUDE_MQTT
#    include "ESP8266WiFi.h"
#    include "endpoints.h"
#    include "environment.h"
#    include "version.h"
#    include "z906remote.h"
#    include <PubSubClient.h>
#endif

namespace z906remote::mqtt {

#ifdef INCLUDE_MQTT
    WiFiClient   espClient;
    PubSubClient mqttClient(espClient);
    uint32_t     lastUpdate = 0;
    uint32_t     timerDelay = 300000;
    char         topic[sizeof(MQTT_UID) + 47];
    char         deviceRoot[sizeof(MQTT_UID) + 12];

    constexpr uint8_t bitsPerAction(uint8_t actionCount) {
        return 32 / actionCount;
    }

    constexpr uint32_t packActions(const uint8_t *actions, uint8_t actionCount) {
        uint32_t packed = 0;
        uint8_t  bits   = bitsPerAction(actionCount);
        uint32_t mask   = (1U << bits) - 1;

        for (uint8_t i = 0; i < actionCount; i++) {
            packed |= (actions[i] & mask) << (i * bits);
        }
        return packed;
    }

    inline uint8_t unpackAction(uint32_t packed, uint8_t index, uint8_t actionCount) {
        const uint8_t  bits = bitsPerAction(actionCount);
        const uint32_t mask = (1UL << bits) - 1UL;
        return static_cast<uint8_t>((packed >> (index * bits)) & mask);
    }

    constexpr uint8_t POWER_ACTIONS[2]  = {PWM_OFF, PWM_ON};
    constexpr uint8_t MUTE_ACTIONS[2]   = {MUTE_OFF, MUTE_ON};
    constexpr uint8_t DOLBY_ACTIONS[2]  = {DISABLE_EFFECT_51, SELECT_EFFECT_51};
    constexpr uint8_t INPUT_ACTIONS[6]  = {SELECT_INPUT_1, SELECT_INPUT_2,
                                           SELECT_INPUT_3, SELECT_INPUT_4,
                                           SELECT_INPUT_5, SELECT_INPUT_AUX};
    constexpr uint8_t EFFECT_ACTIONS[4] = {SELECT_EFFECT_3D, SELECT_EFFECT_41,
                                           SELECT_EFFECT_21, SELECT_EFFECT_NO};

    constexpr const char *INPUT_OPTIONS[] = {"Input 1", "Input 2", "Input 3",
                                             "Input 4", "Input 5", "Input AUX"};

    constexpr const char *EFFECT_OPTIONS[] = {"3D", "4.1", "2.1", "Off"};

    constexpr char   HA_DISCOVERY[] = "homeassistant/%s/%s_%s/config";
    constexpr char   HA_DEVICE[]    = ENV_MQTT_UID "/%s";
    constexpr char   MQTT_DEVICE[]  = ENV_MQTT_UID "/";
    constexpr char   MQTT_STATUS[]  = ENV_MQTT_UID "/status";
    constexpr size_t PREFIX_LEN     = sizeof(MQTT_DEVICE) - 1;
    constexpr char   MQTT_SET[]     = ENV_MQTT_UID "/%s/set";
    constexpr char   MQTT_STATE[]   = ENV_MQTT_UID "/%s/state";

    constexpr EntityConfig ENTITIES[] = {
        {EntityType::Switch, "power", "Power", "mdi:power", RunCommand,
         packActions(POWER_ACTIONS, 2), 2, nullptr, &Z906::t_packetdata::stby},
        {EntityType::Switch, "mute", "Mute", "mdi:volume-off", RunCommand,
         packActions(MUTE_ACTIONS, 2), 2, nullptr, &Z906::t_packetdata::muted},
        {EntityType::Switch, "dolby", "Dolby", "mdi:dolby", RunCommand,
         packActions(DOLBY_ACTIONS, 2), 2, nullptr, &Z906::t_packetdata::decode_mode},
        {EntityType::Number, "main", "Main Volume", "mdi:volume-high", SetValue,
         MAIN_LEVEL, 1, nullptr, &Z906::t_packetdata::main_level},
        {EntityType::Number, "sub", "Sub Volume", "mdi:speaker", SetValue,
         SUB_LEVEL, 1, nullptr, &Z906::t_packetdata::sub_level},
        {EntityType::Number, "center", "Center Volume", "mdi:soundbar",
         SetValue, CENTER_LEVEL, 1, nullptr, &Z906::t_packetdata::center_level},
        {EntityType::Number, "rear", "Rear Volume", "mdi:speaker-multiple",
         SetValue, REAR_LEVEL, 1, nullptr, &Z906::t_packetdata::rear_level},
        {EntityType::Select, "input", "Input Source", "mdi:audio-input-rca", SelectInput,
         packActions(INPUT_ACTIONS, 6), 6, INPUT_OPTIONS, &Z906::t_packetdata::current_input},
        {EntityType::Select, "effect", "Effect Mode", "mdi:surround-sound",
         RunCommand, packActions(EFFECT_ACTIONS, 4), 4, EFFECT_OPTIONS},
        {EntityType::Sensor, "temperature", "Temperature", "mdi:thermometer",
         RunFunction, Temperature, 1},
        {EntityType::Button, "main/up", "Main +", "mdi:volume-plus", RunCommand,
         LEVEL_MAIN_UP, 1, nullptr, &Z906::t_packetdata::main_level},
        {EntityType::Button, "main/down", "Main -", "mdi:volume-minus", RunCommand,
         LEVEL_MAIN_DOWN, 1, nullptr, &Z906::t_packetdata::main_level},
        {EntityType::Button, "sub/up", "Sub +", "mdi:volume-plus", RunCommand,
         LEVEL_SUB_UP, 1, nullptr, &Z906::t_packetdata::sub_level},
        {EntityType::Button, "sub/down", "Sub -", "mdi:volume-minus",
         RunCommand, LEVEL_SUB_DOWN, 1, nullptr, &Z906::t_packetdata::sub_level},
        {EntityType::Button, "center/up", "Center +", "mdi:volume-plus", RunCommand,
         LEVEL_CENTER_UP, 1, nullptr, &Z906::t_packetdata::center_level},
        {EntityType::Button, "center/down", "Center -", "mdi:volume-minus", RunCommand,
         LEVEL_CENTER_DOWN, 1, nullptr, &Z906::t_packetdata::center_level},
        {EntityType::Button, "rear/up", "Rear +", "mdi:volume-plus", RunCommand,
         LEVEL_REAR_UP, 1, nullptr, &Z906::t_packetdata::rear_level},
        {EntityType::Button, "rear/down", "Rear -", "mdi:volume-minus", RunCommand,
         LEVEL_REAR_DOWN, 1, nullptr, &Z906::t_packetdata::rear_level}};

    uint8_t findOptionIndex(const EntityConfig &e, const char *payload) {
        for (uint8_t i = 0; i < e.count; i++) {
            if (strcmp(payload, e.options[i]) == 0)
                return i;
        }
        return e.count;
    }

    void publishState(const EntityConfig &e, const char *value) {
        snprintf(topic, sizeof(topic), MQTT_STATE, e.object_id);

        mqttClient.publish(topic, value);
    }

    void publishState(const EntityConfig &e, uint32_t value) {
        char payload[12];
        char object[7];

        if (e.type == EntityType::Number) {
            snprintf(topic, sizeof(topic), MQTT_STATE, e.object_id);
            value = ((static_cast<uint16_t>(value) * 255 + 21) / 43);
        } else if (e.type == EntityType::Button) {
            value = ((static_cast<uint16_t>(value) * 255 + 21) / 43);
            const char *start = e.object_id;
            const char *end   = strrchr(start, '/');

            if (!end)
                return;

            size_t len = end - start;
            memcpy(object, start, len);
            object[len] = '\0';
            snprintf(topic, sizeof(topic), MQTT_STATE, object);
        } else {
            snprintf(topic, sizeof(topic), MQTT_STATE, e.object_id);
            if (e.type == EntityType::Switch &&
                !strncmp(e.object_id, ENTITIES[0].object_id, 5)) {
                value = !value;
            }
        }
        snprintf(payload, sizeof(payload), "%d", value);

        mqttClient.publish(topic, payload);
    }

    void publishState(const EntityConfig &e) {
        if (e.type == EntityType::Select) {
            if (e.dataMember) {
                publishState(e, INPUT_OPTIONS[LOGI.get_data().*(e.dataMember)]);
            } else {
                publishState(e, EFFECT_OPTIONS[LOGI.current_effect()]);
            }
        } else if (e.dataMember) {
            publishState(e, LOGI.get_data().*(e.dataMember));
        }
    }

    void handleEntityCommand(const EntityConfig &e, const char *payload) {
        uint8_t index;

        switch (e.type) {
        case EntityType::Switch: {
            index = static_cast<uint8_t>(atoi(payload) % e.count);
            LOGI.cmd(unpackAction(e.packedActions, index, e.count));
            break;
        }
        case EntityType::Number: {
            uint8_t parsedValue = 0;
            if (validate_input_value(atoi(payload), parsedValue)) {
                LOGI.cmd(static_cast<uint8_t>(e.packedActions), parsedValue);
            }
            break;
        }
        case EntityType::Select: {
            index = findOptionIndex(e, payload);
            if (index < e.count) {
                switch (e.endpointType) {
                case EndpointType::SelectInput:
                    LOGI.input(unpackAction(e.packedActions, index, e.count));
                    broadcastStatus();
                    break;
                case EndpointType::RunCommand:
                    LOGI.cmd(unpackAction(e.packedActions, index, e.count));
                    break;
                default:
                    return;
                }
            }
            break;
        }
        case EntityType::Button:
            LOGI.cmd(static_cast<uint8_t>(e.packedActions));
            break;
        case EntityType::Sensor:
            break;
        }
    }

    void onMqttMessage(char *ptopic, byte *payload, unsigned int length) {
        char        buffer[32];
        const char *objectId = ptopic + PREFIX_LEN;
        const char *slash    = strrchr(objectId, '/');
        size_t      objectLen;

        if (length >= 32)
            return;

        memcpy(buffer, payload, length);
        buffer[length] = '\0';

        if (strncmp(ptopic, MQTT_DEVICE, PREFIX_LEN) != 0)
            return;
        if (!slash || strcmp(slash, "/set") != 0)
            return;

        objectLen = slash - objectId;
        for (const auto &e : ENTITIES) {
            if (strncmp(objectId, e.object_id, objectLen) == 0) {
                handleEntityCommand(e, buffer);
                LOGI.request(VERSION);
                publishState(e);
            }
        }
    }

    void updateStates() {
        unsigned long now = millis();
        if (!isConnected())
            return;
        if (now - lastUpdate > timerDelay) {
            lastUpdate = now;
            LOGI.request(VERSION);
            for (auto &e : ENTITIES) {
                if (e.type == EntityType::Sensor) {
                    const uint8_t value = LOGI.main_sensor();
                    publishState(e, value);
                } else {
                    publishState(e);
                }
                delay(100);
            }
        }
    }

    bool publishAllDiscovery() {
        JsonWriter jw;

        if (!isConnected())
            return false;

        for (const auto &e : ENTITIES) {
            snprintf(topic, sizeof(topic), HA_DISCOVERY, toString(e.type),
                     MQTT_UID, e.object_id);
            snprintf(deviceRoot, sizeof(deviceRoot), HA_DEVICE, e.object_id);
            jw.init(JSON_BUFFER, sizeof(JSON_BUFFER));

            jw.lit("{\"avty_t\":");
            jw.flash(MQTT_STATUS);
            jw.lit(",\"dev\":{\"ids\":");
            jw.flash(MQTT_UID);
            jw.lit(",\"name\":\"Z906 Remote\"");
            jw.lit(",\"mdl\":\"Z906\"");
            jw.lit(",\"mf\":\"Logitech\"");
            jw.lit(",\"sw\":");
            jw.flash(FIRMWARE_VERSION);
            jw.lit("},\"~\":");
            jw.quote(deviceRoot);
            jw.lit(",\"name\":");
            jw.quote(e.name);
            jw.lit(",\"uniq_id\":");
            jw.quote(e.object_id);
            jw.lit(",\"ic\":");
            jw.quote(e.icon);

            switch (e.type) {
            case EntityType::Switch:
                jw.lit(",\"cmd_t\":\"~/set\",\"stat_t\":\"~/"
                       "state\",\"pl_on\":\"1\",\"pl_off\":\"0\"\"stat_on\":"
                       "\"1\",\"stat_off\":\"0\"");
                break;
            case EntityType::Number:
                jw.lit(",\"cmd_t\":\"~/set\",\"stat_t\":\"~/"
                       "state\",\"mode\":\"slider\",\"min\":0,\"max\":255,"
                       "\"step\":6");
                break;
            case EntityType::Select:
                jw.lit(",\"cmd_t\":\"~/set\",\"stat_t\":\"~/state\"");
                jw.lit(",\"ops\":[");
                for (int i = 0; i < e.count; i++) {
                    jw.quote(e.options[i]);
                    if (i < e.count - 1)
                        jw.comma();
                }
                jw.buf[jw.len++] = ']';
                break;
            case EntityType::Sensor:
                jw.lit(",\"stat_t\":\"~/"
                       "state\",\"dev_cla\":\"temperature\",\"stat_cla\":"
                       "\"measurement\"");
                break;
            case EntityType::Button:
                jw.lit(",\"cmd_t\":\"~/set\",\"pl_prs\":\"1\"");
                break;
            }
            jw.buf[jw.len++] = '}';
            jw.buf[jw.len]   = 0;

            mqttClient.publish(topic, JSON_BUFFER, true);
            delay(100);
        }
        return true;
    }

    bool publishAllState() {
        if (!isConnected())
            return false;
        for (auto &e : ENTITIES) {
            publishState(e);
            delay(100);
        }
        return true;
    }

    void subscribeToEntityTopics() {
        for (const auto &e : ENTITIES) {
            if (e.type == EntityType::Sensor)
                continue;
            snprintf(topic, sizeof(topic), MQTT_SET, e.object_id);
            mqttClient.subscribe(topic);
        }
    }

    void init() {
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        mqttClient.setCallback(onMqttMessage);
        mqttClient.setKeepAlive(15);
        mqttClient.setSocketTimeout(3);
    }

    void connect() {
        static bool     execOnce = false;
        static uint32_t last     = 0;
        unsigned long   now      = millis();

        if (now - last > 60000) {
            last = now;
            if (mqttClient.connect(MQTT_UID, MQTT_USER, MQTT_PASS, MQTT_STATUS,
                                   0, true, "offline")) {
                mqttClient.setBufferSize(sizeof(JSON_BUFFER));
                subscribeToEntityTopics();
                if (!execOnce) {
                    execOnce = publishAllDiscovery() && publishAllState();
                }
                mqttClient.publish(MQTT_STATUS, "online", true);
            } else {
                broadcastLog("Failed to connect to MQTT broker", LogLevel::warn);
            }
        }
    }

    bool isConnected() { return mqttClient.connected(); }

    void loop() { mqttClient.loop(); }
#else
    void init() {}
    void connect() {}
    bool isConnected() { return false; }
    void loop() {}
    void updateStates() {}
#endif

} // namespace z906remote::mqtt
