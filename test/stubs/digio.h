#ifndef TEST_STUB_DIGIO_H
#define TEST_STUB_DIGIO_H

class StubDigIoPin {
public:
    void Set() { state = true; }
    void Clear() { state = false; }
    bool Get() const { return state; }

private:
    bool state = false;
};

struct DigIo {
    static StubDigIoPin ignition_in;
    static StubDigIoPin ready_safety_in;
    static StubDigIoPin ready_out;
    static StubDigIoPin condition_out;
    static StubDigIoPin vcu_out;
};

#endif
