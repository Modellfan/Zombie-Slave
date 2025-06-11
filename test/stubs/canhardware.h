#ifndef CANHARDWARE_H
#define CANHARDWARE_H
class CanHardware {
public:
    virtual ~CanHardware() = default;
    virtual void RegisterUserMessage(int) {}
};
#endif
