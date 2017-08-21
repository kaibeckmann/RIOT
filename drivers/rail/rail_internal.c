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
    RADIO_PAInit_t paInit;
    
    // necessary?
    //SYSTEM_ChipRevision_TypeDef chipRev;
    //SYSTEM_ChipRevisionGet(&chipRev);
    
    // Init DCDC regulator and HFXO with WSTK radio board specific parameters
#ifdef EMU_DCDCINIT_WSTK_DEFAULT
    EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_WSTK_DEFAULT;
    EMU_DCDCInit(&dcdcInit);
    DEBUG("EMU_DCDCInit done\n");
#else
    DEBUG("EMU_DCDCPowerOff pre\n");
    EMU_DCDCPowerOff();
    DEBUG("EMU_DCDCPowerOff done\n");
#endif
//    CMU_HFXOInit(&hfxoInit);
//    DEBUG("CMU_HFXOInit done\n");
    SystemHFXOClockSet(params->XTAL_frequency);
    DEBUG("SystemHFXOClockSet done\n");
    // todo check if activated
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
    paInit = (RADIO_PAInit_t) RADIO_PA_2P4_INIT;
    
    if (!RADIO_PA_Init(&paInit))
    {
        // Error: The PA could not be initialized due to an improper configuration.
        // Please ensure your configuration is valid for the selected part.
        LOG_ERROR("Can't init rail radio PM\n");
        while (1);
    }
    DEBUG("RADIO_PA_Init done\n");
    
    // Initialize other chip clocks
  //  CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
  //  DEBUG("CMU_OscillatorEnable done\n");
  //  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
  //  DEBUG("CMU_ClockSelectSet done\n");
  //  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);
  //  DEBUG("CMU_ClockSelectSet done\n");
    CMU_ClockEnable(cmuClock_CORELE, true);
    DEBUG("CMU_ClockEnable done\n");

    
    return 0;
}
