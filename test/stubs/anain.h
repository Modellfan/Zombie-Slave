#ifndef TEST_STUB_ANAIN_H
#define TEST_STUB_ANAIN_H

class StubAnaInChannel {
public:
    void Set(float newValue) { value = newValue; }
    float Get() const { return value; }

private:
    float value = 0.0f;
};

struct AnaIn {
    static StubAnaInChannel dc_power_supply;
};

#endif
