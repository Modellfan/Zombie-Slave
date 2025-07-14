#ifndef ERRORMESSAGE_H
#define ERRORMESSAGE_H
class ErrorMessage {
public:
    static void Post(int) {}
    static void PrintAllErrors() {}
    static void SetTime(int) {}
};

enum {
    ERR_BMS_TIMEOUT,
    ERR_BMS_FAULT,
    ERR_BMS_CONTACTOR_FAULT
};
#endif
