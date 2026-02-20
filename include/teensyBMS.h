#ifndef TEENSYBMS_H
#define TEENSYBMS_H

#include "bms.h"
#include "canhardware.h"
#include "errormessage.h"
#include <stdint.h>

#define BMS_TIMEOUT_TICKS 3 // 300ms @ 100ms cycle

class TeensyBMS : public BMS {
public:
    void SetCanInterface(CanHardware* c) override;
    void DecodeCAN(int id, uint8_t* data) override;
    float MaxChargeCurrent() override;
    void Task100Ms() override;

private:
    void parseMsg1(uint8_t* data);
    void parseMsg2(uint8_t* data);
    void parseMsg3(uint8_t* data);
    void parseMsg4(uint8_t* data);
    void parseMsg5(uint8_t* data);
    bool checkCrc(uint8_t* data);

    int timeoutCounter = 0;

    float vMin = 0;
    float vMax = 0;
    float tMin = 0;
    float tMax = 0;
    float packVoltage = 0;
    float deltaVoltageMv = 0;
    float balancingVoltage = 0;
    float actualCurrent = 0;
    float maxChargeCurrent = 0;
    float maxDischargeCurrent = 0;
    float packPower = 0;
    float soc = 0;
    float soh = 0;
    float averageEnergyPerHour = 0;
    float remainingEnergyKWh = 0;

    float remainingTimeSeconds = 0;

    uint8_t state = 0;
    uint8_t dtc = 0;
    uint8_t balancingStatus = 0;
    bool balancingActive = false;
    bool anyBalancing = false;

    // Shutdown handshake
    bool shutdownRequest = false;
    bool shutdownReady = false;
    bool shutdownAck = false;

    // Contactor manager
    uint8_t contactorState = 0;
    bool contactorNegativeInput = false;
    bool contactorPositiveInput = false;
    bool contactorPrechargeInput = false;
    bool contactorSupplyAvailable = false;

    uint8_t txCounter = 0;
};

#endif // TEENSYBMS_H
