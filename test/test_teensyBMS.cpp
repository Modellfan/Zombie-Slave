#include "teensyBMS.h"
#include <cassert>

class MockCanHardware : public CanHardware {
public:
    int count = 0;
    void RegisterUserMessage(int) override { count++; }
    int sendCount = 0;
    int lastId = 0;
    uint8_t lastData[3] = {0};
    void Send(int id, uint8_t* data, int) override {
        sendCount++; lastId = id;
        lastData[0] = data[0];
        lastData[1] = data[1];
        lastData[2] = data[2];
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
        assert(can.count == 7);

        Param::SetInt(Param::LVDU_vehicle_state, 3);
        Param::SetInt(Param::LVDU_forceVCUsShutdown, 1);
        Param::SetInt(Param::LVDU_connectHVcommand, 0);
        for (int i = 0; i < 5; ++i)
            bms.Task100Ms();

        assert(can.sendCount == 5);
        assert(can.lastId == 0x437);
        assert(can.lastData[0] == 3);
        assert(can.lastData[1] == 1);
        assert(can.lastData[2] == 0);
    }
    return 0;
}
