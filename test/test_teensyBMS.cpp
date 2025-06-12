#include "teensyBMS.h"
#include <cassert>

class MockCanHardware : public CanHardware {
public:
    int count = 0;
    void RegisterUserMessage(int) override { count++; }
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
    }
    return 0;
}
