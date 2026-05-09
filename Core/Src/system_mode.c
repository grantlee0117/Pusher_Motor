#include "system_mode.h"

static volatile SystemMode_t current_mode = SYSTEM_MODE_BOOT;

void system_mode_init(void)
{
    current_mode = SYSTEM_MODE_BOOT;
}

SystemMode_t system_mode_get(void)
{
    return current_mode;
}

uint32_t system_mode_set(SystemMode_t mode)
{
    if (mode > SYSTEM_MODE_ERROR)
    {
        return 1;
    }

    current_mode = mode;
    return 0;
}

const char *system_mode_to_string(SystemMode_t mode)
{
    switch (mode)
    {
        case SYSTEM_MODE_BOOT:
            return "BOOT";
        case SYSTEM_MODE_IDLE:
            return "IDLE";
        case SYSTEM_MODE_RUNNING:
            return "RUNNING";
        case SYSTEM_MODE_SERVICE:
            return "SERVICE";
        case SYSTEM_MODE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

uint8_t system_mode_can_configure(void)
{
    SystemMode_t mode = system_mode_get();
    return (mode == SYSTEM_MODE_IDLE || mode == SYSTEM_MODE_SERVICE) ? 1U : 0U;
}

uint8_t system_mode_can_start(void)
{
    return (system_mode_get() == SYSTEM_MODE_IDLE) ? 1U : 0U;
}

uint8_t system_mode_can_direct_pwm(void)
{
    return (system_mode_get() == SYSTEM_MODE_SERVICE) ? 1U : 0U;
}

uint32_t system_mode_enter_service(void)
{
    if (system_mode_get() != SYSTEM_MODE_IDLE)
    {
        return 1;
    }

    return system_mode_set(SYSTEM_MODE_SERVICE);
}

uint32_t system_mode_exit_service(void)
{
    if (system_mode_get() != SYSTEM_MODE_SERVICE)
    {
        return 1;
    }

    return system_mode_set(SYSTEM_MODE_IDLE);
}
