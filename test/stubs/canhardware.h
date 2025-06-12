#ifndef CANHARDWARE_H
#define CANHARDWARE_H
class CanHardware {
public:
    virtual ~CanHardware() = default;
    virtual void RegisterUserMessage(int) {}
    virtual void Send(int, uint8_t*, int) {}
};
#endif
