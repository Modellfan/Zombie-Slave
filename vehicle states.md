stateDiagram-v2
    [*] --> Sleep

    Sleep --> Standby : ignition_on / charger / remote
    Sleep --> Conditioning : remote_start_command
    Sleep --> Charge : charger_plugged_in

    Standby --> Ready : ignition_on
    Standby --> Charge : charger_plugged_in
    Standby --> Sleep : timeout

    Ready --> Drive : brake_and_start_pressed
    Ready --> Charge : charger_plugged_in
    Ready --> Conditioning : ignition_off

    Drive --> Conditioning : ignition_off
    Drive --> Charge : charger_plugged_in
    Drive --> Error : critical_fault_detected
    Drive --> Limp_Home : degraded_fault_detected

    Conditioning --> Sleep : thermal_task_completed
    Conditioning --> Ready : ignition_on
    Conditioning --> Charge : charger_plugged_in
    Conditioning --> Error : critical_fault_detected

    Charge --> Conditioning : charger_unplugged_or_done
    Charge --> Error : critical_fault_detected

    Error --> Sleep : power_cycle_12v

    Limp_Home --> Conditioning : ignition_off
    Limp_Home --> Error : fault_escalates

    state Sleep {
        note right of Sleep : All systems OFF\nWake on remote/app
    }
    state Standby {
        note right of Standby : Light wake\nVCU + BMS only
    }
    state Ready {
        note right of Ready : HV ON\nNo torque
    }
    state Drive {
        note right of Drive : Full drive mode
    }
    state Conditioning {
        note right of Conditioning : Thermal control\nNo torque
    }
    state Charge {
        note right of Charge : External charging
    }
    state Error {
        note right of Error : Fault state\nHV off
    }
    state Limp_Home {
        note right of Limp_Home : Degraded drive
    }
