#ifndef MVCUINTEGRATION_H
#define MVCUINTEGRATION_H

#include <stdint.h>
#include "canhardware.h"

class mVCUIntegration
{
public:
    void SetCanInterface(CanHardware* c);
    void Task100Ms();

private:
    CanHardware* can = nullptr;
};

#endif // MVCUINTEGRATION_H
