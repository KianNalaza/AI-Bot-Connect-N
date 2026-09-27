#pragma once
#include <stdint.h>

class Messages {
public:
    enum MessageType : uint8_t {
        START_GAME     = 0b001,
        START_GAME_ACK = 0b010,
        MOVE           = 0b100,
        UNKNOWN_TYPE   = 0xFF
    };

    enum MoveTimeout : uint8_t {
        MTO_100MS = 0b00,
        MTO_200MS = 0b01,
        MTO_400MS = 0b10,
        MTO_600MS = 0b11
    };

    static constexpr uint8_t INVALID_MOVE_COL = 0b11111;

    struct StartGameFields {
        MoveTimeout mto;
        uint8_t numCols;   // 4 bits
        uint8_t numRows;   // 4 bits
        uint8_t inALine;   // 3 bits
    };

    struct StartGameAckFields {
        uint16_t botId;    // 13 bits
        //uint16_t botId = 708
    };

    struct MoveFields {
        uint8_t sequenceNum; // 8 bits
        uint8_t moveCol;     // 5 bits
    };

    // -------------------------
    // Message creation
    // -------------------------

    static uint16_t makeStartGame(MoveTimeout mto,
                                  uint8_t numCols,
                                  uint8_t numRows,
                                  uint8_t inALine)
    {
        return (uint16_t(START_GAME) << 13) |
               ((uint16_t(mto)     & 0x03) << 11) |
               ((uint16_t(numCols) & 0x0F) << 7)  |
               ((uint16_t(numRows) & 0x0F) << 3)  |
               ((uint16_t(inALine) & 0x07) << 0);
    }

    static uint16_t makeStartGameAck(uint16_t botId)
    {
        return (uint16_t(START_GAME_ACK) << 13) |
               (botId & 0x1FFF);
    }

    static uint16_t makeMove(uint8_t sequenceNum, uint8_t moveCol)
    {
        return (uint16_t(MOVE) << 13) |
               ((uint16_t(sequenceNum) & 0xFF) << 5) |
               ((uint16_t(moveCol)     & 0x1F) << 0);
    }

    static uint16_t makeInvalidMove(uint8_t sequenceNum)
    {
        return makeMove(sequenceNum, INVALID_MOVE_COL);
    }

    // -------------------------
    // Message type
    // -------------------------

    static MessageType getType(uint16_t pdu)
    {
        uint8_t typeBits = (pdu >> 13) & 0x07;

        switch (typeBits) {
            case START_GAME:     return START_GAME;
            case START_GAME_ACK: return START_GAME_ACK;
            case MOVE:           return MOVE;
            default:             return UNKNOWN_TYPE;
        }
    }

    // -------------------------
    // Message parsing
    // -------------------------

    static bool parseStartGame(uint16_t pdu, StartGameFields& out)
    {
        if (getType(pdu) != START_GAME) {
            return false;
        }

        out.mto      = static_cast<MoveTimeout>((pdu >> 11) & 0x03);
        out.numCols  = (pdu >> 7) & 0x0F;
        out.numRows  = (pdu >> 3) & 0x0F;
        out.inALine  = (pdu >> 0) & 0x07;
        return true;
    }

    static bool parseStartGameAck(uint16_t pdu, StartGameAckFields& out)
    {
        if (getType(pdu) != START_GAME_ACK) {
            return false;
        }

        out.botId = pdu & 0x1FFF;
        return true;
    }

    static bool parseMove(uint16_t pdu, MoveFields& out)
    {
        if (getType(pdu) != MOVE) {
            return false;
        }

        out.sequenceNum = (pdu >> 5) & 0xFF;
        out.moveCol     = (pdu >> 0) & 0x1F;
        return true;
    }

    // -------------------------
    // Helpers
    // -------------------------

    static bool isInvalidMove(uint16_t pdu)
    {
        if (getType(pdu) != MOVE) {
            return false;
        }
        return ((pdu & 0x1F) == INVALID_MOVE_COL);
    }

    static uint16_t timeoutToMs(MoveTimeout mto)
    {
        switch (mto) {
            case MTO_100MS: return 100;
            case MTO_200MS: return 200;
            case MTO_400MS: return 400;
            case MTO_600MS: return 600;
            default:        return 0;
        }
    }

    static bool isValidBoardConfig(uint8_t numCols, uint8_t numRows, uint8_t inALine)
    {
        // Spec says board size can range from 6x6 to 15x15
        // and connect target can range from 3 to 7.
        return (numCols >= 6 && numCols <= 15) &&
               (numRows >= 6 && numRows <= 15) &&
               (inALine >= 3 && inALine <= 7);
    }
};