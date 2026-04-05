#ifndef MVCUINTEGRATION_H
#define MVCUINTEGRATION_H

#include <stdint.h>
#include "canhardware.h"

class mVCUIntegration
{
public:
    void SetCanInterface(CanHardware* c);
    void DecodeCAN(int id, uint8_t* data, uint8_t dlc);
    void Task100Ms();

private:
    bool CheckCrc(const uint8_t* data) const;
    bool ValidateCounter(uint8_t counter);

    CanHardware* can = nullptr;
    uint8_t txCounter = 0;
    bool heaterCanCloseRequest = false;
    bool haveSeenValidCounter = false;
    uint8_t lastRxCounter = 0;
    uint8_t rxTimeoutTicks = 0;
};

#endif // MVCUINTEGRATION_H
