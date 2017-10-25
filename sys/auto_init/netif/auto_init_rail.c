

#ifdef  MODULE_RAIL

#include <stdint.h>

#define ENABLE_DEBUG  (1)
#include "debug.h"

#include "log.h"
#include "net/gnrc/netdev2.h"
#include "net/gnrc/netdev2/ieee802154.h"
#include "net/gnrc.h"

#include "rail_drv.h"
#include "rail_params.h"

#define RAIL_MAC_STACKSIZE      (THREAD_STACKSIZE_DEFAULT)
#ifndef RAIL_MAC_PRIO
#define RAIL_MAC_PRIO           (GNRC_NETDEV2_MAC_PRIO)
#endif


// there could be more interfaces, if BLE and 802.15.4 are used together, or the 
// 2.4 GHz and the 868 MHz transceiver?

#define RAIL_IFACE_NUM 1 // todo calc number?
#define RAIL_802154_2P4GHZ_PARAM_INDEX 0

static rail_t rail_802154_2p4ghz_dev;
static gnrc_netdev2_t gnrc_802154_2p4ghz_adpt; // todo what does it do?
static char _rail_802154_2p4ghz_stack[RAIL_MAC_STACKSIZE];


void auto_init_rail(void)
{
    int ret;
    // there is only on iface atm
    
    DEBUG("[auto_init_netif] called\n");
    
    // 802.15.4 2.4ghz
    const rail_params_t *p = &rail_params[RAIL_802154_2P4GHZ_PARAM_INDEX];
    
    // init rail driver
    
    rail_setup(&rail_802154_2p4ghz_dev, (rail_params_t*) p);
    
    // init ieee802154 layer
    ret = gnrc_netdev2_ieee802154_init(&gnrc_802154_2p4ghz_adpt,
                                           (netdev2_ieee802154_t *)&rail_802154_2p4ghz_dev);
                                           
    if (ret < 0) {
        LOG_ERROR("[auto_init_netif] error initializing rail radio 802154_2P4GHZ");
        return ;
    }
    // init netdev2
    
    kernel_pid_t pid = gnrc_netdev2_init(
                              _rail_802154_2p4ghz_stack,
                              RAIL_MAC_STACKSIZE,
                              RAIL_MAC_PRIO,
                              "rail 802.15.4 2.4GHz",
                              &gnrc_802154_2p4ghz_adpt);
    
    
    if (pid == KERNEL_PID_UNDEF) {
        LOG_ERROR("gnrc_netdev2_init for rail 802.15.4 2.4GHz failed\n");
        return ;
    }
    
}

#else
typedef int dont_be_pedantic;

#endif   /* ----- #ifdef MODULE_RAIL_INC  ----- */
