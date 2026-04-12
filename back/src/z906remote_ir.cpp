#include "z906remote_ir.h"
#ifdef INCLUDE_IR
#    include "z906remote.h"
#    include <IRrecv.h>
#    include <IRremoteESP8266.h>
#    include <IRutils.h>
#    include <Z906.h>
#endif

namespace z906remote::infrared {
#ifdef INCLUDE_IR

    IRrecv         IRRECV(IR_PIN);
    decode_results results;
    uint32_t       lastCode       = 0;
    uint32_t       firstPressTime = 0;
    uint32_t       lastRepeatTime = 0;

    /**
     *
     */
    void enableIR() { IRRECV.enableIRIn(); }

    /**
     *
     */
    void handleIRCommand(uint32_t code) {
        JsonWriter               jw;
        bool                     sendStatus = LOGI.request(VERSION);
        const Z906::t_packetdata packet     = LOGI.get_data();

        if (!sendStatus)
            return;

        jw.init(JSON_BUFFER, sizeof(JSON_BUFFER));
        jw.lit("{\"status\":{");

        switch (code) {
        case IR_POWER:
            packet.stby ? LOGI.on() : LOGI.off();
            jw.lit("\"stby\":");
            jw.writeBool(packet.stby);
            break;
        case IR_MUTE:
            LOGI.cmd(LOGI.muted_state() ? MUTE_OFF : MUTE_ON);
            jw.lit("\"muted\":");
            jw.writeBool(LOGI.muted_state());
            break;
        case IR_LEVEL:
            break;
        case IR_PLUS:
            LOGI.cmd(LEVEL_MAIN_UP);
            jw.lit("\"main_level\":");
            jw.writeNumber(packet.main_level);
            jw.comma();
            jw.lit("\"muted\":");
            jw.writeBool(LOGI.muted_state());
            break;
        case IR_EFFECT:
            break;
        case IR_MINUS:
            LOGI.cmd(LEVEL_MAIN_DOWN);
            jw.lit("\"main_level\":");
            jw.writeNumber(packet.main_level);
            jw.comma();
            jw.lit("\"muted\":");
            jw.writeBool(LOGI.muted_state());
            break;
        case IR_INPUT1:
            break;
        case IR_INPUT2:
            break;
        case IR_INPUT3:
            break;
        case IR_INPUT4:
            break;
        case IR_INPUT5:
            break;
        case IR_AUX:
            break;
        case IR_TEST:
            break;
        default:
            break;
        }

        jw.lit("}}");
        jw.buf[jw.len] = 0;

        broadcastPartialStatus(JSON_BUFFER);
    }

    /**
     *
     */
    void processIR() {
        uint32_t timeHeld;
        uint32_t repeatDelay;
        uint32_t now = millis();

        if (!IRRECV.decode(&results)) {
            return;
        }

        if (results.decode_type != NEC) {
            IRRECV.resume();
            return;
        }

        if (!results.repeat && results.address == IR_ADDRESS) {
            lastCode       = results.command;
            firstPressTime = now;
            lastRepeatTime = now;
            handleIRCommand(lastCode);
        } else if (results.repeat && lastCode != 0) {
            if (lastCode != IR_MINUS && lastCode != IR_PLUS) {
                IRRECV.resume();
                return;
            }
            timeHeld = now - firstPressTime;

            if (timeHeld > 250) {
                repeatDelay = (timeHeld > 1500) ? 0 : 200;

                if (now - lastRepeatTime >= repeatDelay) {
                    handleIRCommand(lastCode);
                    lastRepeatTime = now;
                }
            }
        }
        IRRECV.resume();
    }
#else
    void enableIR() {}
    void processIR() {}
#endif

} // namespace z906remote::infrared
