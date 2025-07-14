#include "teensyBMS.h"
#include <cassert>

class MockCanHardware : public CanHardware {
public:
    int count = 0;
    void RegisterUserMessage(int) override { count++; }
    int sendCount = 0;
    int lastId = 0;
    uint8_t lastData[8] = {0};
    int lastLen = 0;
    void Send(int id, uint8_t* data, int len) override {
        sendCount++; lastId = id; lastLen = len;
        for (int i = 0; i < len && i < 8; ++i)
            lastData[i] = data[i];
    }
};

class TestTeensyBMS : public TeensyBMS {
public:
    CanHardware* GetCan() const { return can; }
};

int main() {
    {
        TestTeensyBMS bms;
        bms.SetCanInterface(nullptr);
        assert(bms.GetCan() == nullptr);
    }
    {
        TestTeensyBMS bms;
        MockCanHardware can;
        bms.SetCanInterface(&can);
        assert(bms.GetCan() == &can);
        assert(can.count == 5);

        Param::SetInt(Param::LVDU_vehicle_state, 3);
        Param::SetInt(Param::LVDU_forceVCUsShutdown, 1);
        Param::SetInt(Param::LVDU_connectHVcommand, 0);
        for (int i = 0; i < 5; ++i)
            bms.Task100Ms();

        assert(can.sendCount == 5);
        assert(can.lastId == 0x437);
        assert(can.lastLen == 8);
        assert(can.lastData[0] == 3);
        assert(can.lastData[1] == 1);
        assert(can.lastData[2] == 0);
        assert(can.lastData[3] == 4);
        assert(can.lastData[4] == 0); // CRC from stub

        // Test DecodeCAN updates parameters
        Param::SetInt(Param::BMS_DecodeCanCalled, 0);
        Param::SetInt(Param::BMS_LastCanId, 0);
        uint8_t data[8] = {0};
        bms.DecodeCAN(0x41A, data);
        assert(Param::GetInt(Param::BMS_DecodeCanCalled) == 1);
        assert(Param::GetInt(Param::BMS_LastCanId) == 0x41A);
    }
    return 0;
}
