#define TIMER

#ifdef TIMER
    #include "bbb_regs.h"
    #include "hw_types.h"

    void timerIrqHandler(void);
    void timerSetup(void);
    void timerEnable();
    void timerDisable();
    void delay(unsigned int mSec);
#endif