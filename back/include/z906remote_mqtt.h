#pragma once
#include <Arduino.h>
#include <Z906.h>
#include <endpoints.h>

namespace z906remote::mqtt {

    enum class EntityType { Switch, Number, Select, Sensor, Button };

    inline const char *toString(EntityType type) {
        switch (type) {
        case EntityType::Switch:
            return "switch";
        case EntityType::Number:
            return "number";
        case EntityType::Select:
            return "select";
        case EntityType::Sensor:
            return "sensor";
        default:
            return "button";
        }
    }

    struct EntityConfig {
        EntityType         type;
        const char        *object_id;
        const char        *name;
        const char        *icon;
        EndpointType       endpointType;
        uint32_t           packedActions;
        uint8_t            count;
        const char *const *options              = nullptr;
        uint8_t Z906::t_packetdata::*dataMember = nullptr;
    };

    bool isConnected();
    void connect();
    void init();
    void loop();
    void updateStates();
} // namespace z906remote::mqtt
