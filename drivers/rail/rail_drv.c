
#include <string.h>

//#include "rail.h"

#include "rail_drv.h"
#include "rail_registers.h"
#include "rail_internal.h"
#include "rail_netdev.h"

#define ENABLE_DEBUG (1)
#include "debug.h"


void rail_setup(rail_t* dev, const rail_params_t* params)
{
    
    netdev2_t *netdev = (netdev2_t *)dev;
    
    // register driver (defined in rail_netdev)
    netdev->driver = &rail_driver;
    
    // init dev 
    memcpy(&dev->params, params, sizeof(rail_params_t));
    
    // ?? todo
    
    DEBUG("rail->setup called\n");
    
    //// init radio
    //RAIL_Init_t railInitParams =
    //{
        //128, // maxPacketLength: UNUSED
        //RADIO_CONFIG_XTAL_FREQUENCY,
        //0
    //};
    //RAIL_RfInit(&railInitParams);
    //RAIL_RfIdle();
}
