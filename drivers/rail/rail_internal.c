#include "rail_internal.h"

#include "em_cmu.h"
#include "em_emu.h"
#include "pa.h"
#include "pti.h"

#include "bspconfig.h"

#include "log.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

int rail_internal_init_radio_hal(const rail_params_t* params)
{
    
    DEBUG("rail_internal_init_radio_hal called\n");
    //CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_WSTK_DEFAULT;
  //  RADIO_PTIInit_t ptiInit = RADIO_PTI_INIT;
    
    // necessary?
    //SYSTEM_ChipRevision_TypeDef chipRev;
    //SYSTEM_ChipRevisionGet(&chipRev);
    
   // RADIO_PTI_Init(&ptiInit);
  //  DEBUG("RADIO_PTI_Init done\n");
    
    /* Switch HFCLK to HFXO and disable HFRCO */
    //CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    //DEBUG("CMU_ClockSelectSet done\n");
   // CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
   // DEBUG("CMU_OscillatorEnable done\n");
 /*   
    // Initialize the PA now that the HFXO is up and the timing is correct
    if (params->BASE_frequency < 1000000000UL) {
#ifdef RADIO_PA_SUBGIG_INIT
        paInit = (RADIO_PAInit_t) RADIO_PA_SUBGIG_INIT;
#endif
    } else {
#ifdef RADIO_PA_2P4_INIT
        
#endif
    }
   */ 
    
       

    
    return 0;
}
