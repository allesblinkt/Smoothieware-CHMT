#include "Watchdog.h"
#include "Kernel.h"

#include "stm32f4xx.h"

#include <mri.h>

#include "gpio.h"
extern GPIO leds[];

// TODO : comment this
// Basically, when stuff stop answering, reset, or enter MRI mode, or something

static WWDG_HandleTypeDef m_wdt_handle;

Watchdog::Watchdog(uint32_t timeout, WDT_ACTION action)
{
    m_wdt_handle.Instance         = WWDG;
    m_wdt_handle.Init.Prescaler   = WWDG_CFR_WDGTB0 | WWDG_CFR_WDGTB1; // prescale /8
    m_wdt_handle.Init.Window      = WWDG_CFR_W; // load max values, still timeout ~ 100ms
    m_wdt_handle.Init.Counter     = WWDG_CR_T;  // TODO rewrite to use IWDG for longer timeouts
    m_wdt_handle.Init.EWIMode     = (action == WDT_MRI) ? WWDG_SR_EWIF : 0;   

    HAL_WWDG_Init(&m_wdt_handle);
    feed();

    if(action == WDT_MRI) {
        // enable the interrupt
        NVIC_EnableIRQ(WWDG_IRQn);
        NVIC_SetPriority(WWDG_IRQn, 1);
    }
}

void Watchdog::feed()
{
    HAL_WWDG_Refresh(&m_wdt_handle);
}

void Watchdog::on_module_loaded()
{
    register_for_event(ON_IDLE);
    feed();
}

void Watchdog::on_idle(void*)
{
    feed();
}


// when watchdog triggers, set a led pattern and enter MRI which turns everything off into a safe state
// TODO handle when MRI is disabled
extern "C" void WWDG_IRQHandler(void)
{
    if(THEKERNEL->is_using_leds()) {
        // set led pattern to show we are in watchdog timeout
        leds[0]= 0;
    //    leds[1]= 1; // chmt has only 1 led
    //    leds[2]= 0;
    //    leds[3]= 1;
    }

    HAL_WWDG_IRQHandler(&m_wdt_handle); // clears int flag
    HAL_WWDG_Refresh(&m_wdt_handle);
    __debugbreak();
}
