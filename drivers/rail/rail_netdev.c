
#include <string.h>

#include "pa.h"
#include "pti.h"
#include "rail.h"
#include "em_core.h"


#include "net/netdev2.h"
#include "net/netopt.h"

#include "ps.h"
#include "xtimer.h"

#include "errno.h"

#include "rail_drv.h"
#include "rail_netdev.h"
#include "rail_internal.h"
#include "rail_registers.h"



#include "log.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

// local declaration of driver methodes
static int _send(netdev2_t* netdev, const struct iovec* vector, unsigned count);
static int _recv(netdev2_t* netdev, void* buf, size_t len, void* info);
static int _init(netdev2_t* netdev);
static void _isr(netdev2_t* netdev);
static int _get(netdev2_t* netdev, netopt_t opt, void* val, size_t max_len);
static int _set(netdev2_t* netdev, netopt_t opt, void* val, size_t len);


static  uint8_t frame[IEEE802154_FRAME_LEN_MAX + 1];


static const RAIL_CsmaConfig_t csmaConfig = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

// local helper functions
netopt_state_t _get_state(rail_t* dev);

const netdev2_driver_t rail_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};




static void _lowlevel_irq_handler(void* arg)
{
    netdev2_t* dev = (netdev2_t*) arg;

    // todo where to set what happend?

    // delegate irq handling to device thread
    if (dev->event_callback) {
        dev->event_callback(dev, NETDEV2_EVENT_ISR);
    }
}

static int _init(netdev2_t* netdev) {
    
    rail_t* dev = (rail_t*) netdev;

    void (*foo)(void*) = _lowlevel_irq_handler;
    
    foo =foo;
    
    DEBUG("rail_netdev->init called\n");
     // set default channel
#if (RAIL_RADIO_BAND == 2400)
    dev->netdev.chan = IEEE802154_DEFAULT_CHANNEL;
#elif (RAIL_RADIO_BAND == 868)
    dev->netdev.chan = 0;
#elif (RAIL_RADIO_BAND == 915)
    dev->netdev.chan = 1;
#endif
   
    int ret;
    
    ret = rail_init(dev);
    
    if (ret < 0) {
        return ret;
    }

    ret = rail_start_rx(dev);
    if (ret < 0) {
        return ret;
    }
//    RAIL_TRANSCEIVER_STATE_TX
    
    
    
    return 0;
}


static int _send(netdev2_t* netdev, const struct iovec* vector, unsigned count) 
{
    DEBUG("rail_netdev->send called\n");
    
    rail_t* dev = (rail_t*) netdev;
    

    // todo check current state, make it depend what to do
    // if tx, return error
    // if init or so return error
    // if calibaration error
    // if idle or rx, send
    //      if waiting for ack
    //        no timeout return error
    //        timeout update stat and continue
    
    // prepare frame, cpy header and payload
    size_t len = 1; // start with 1, first byte have to be the length
    const struct iovec *ptr = vector;
    for (unsigned int i = 0; i < count; i++, ptr++) {
    //    DEBUG("add header %d - len %d sum leng %d \n", i, ptr->iov_len, len);
#ifdef MODULE_NETSTATS_L2
        netdev->stats.tx_bytes += len;
#endif
        memcpy(frame+len, ptr->iov_base, ptr->iov_len);
        len += ptr->iov_len;
    }
    
    // set frame length to first byte (I wonder where it is documented?)
    frame[0] = (uint8_t) len +1;//-1; // todo check if -1 is ok // ggf. +2 for crc?
    
    /*
    for (int j = 0; j < 50; j++)
    {
        frame[j] = 0x23;
    }
    len = 50;
    frame[0] = 50;
    */
    
 //   uint8_t foo[512];

//    (void)foo;
    
    RAIL_TxData_t txData;
    
    txData.dataPtr = frame;
    txData.dataLength = len + 1;
    
    // config tx options
    RAIL_TxOptions_t txOption;
    
    // check if ack req
    if (((netdev2_ieee802154_t *)netdev)->flags & NETDEV2_IEEE802154_ACK_REQ) {
        txOption.waitForAck =  true;
        DEBUG("tx option auto ack\n");
    } else {
        txOption.waitForAck =  false;
        DEBUG("tx option NO auto ack\n");
    }
    txOption.removeCrc = false;  // transceiver should calc and add cnc
    txOption.syncWordId = 0;    // dunno what it does ...
    
 //   CORE_DECLARE_IRQ_STATE; // macht keinen unterschied
 //   CORE_ENTER_CRITICAL();
    
    
  //   RAIL_RfIdle();
 //    RAIL_RxStart(dev->channel);
     
    // tx prepare
    int ret = rail_tx_prepare(dev);
    if (ret != 0) {
        return ret;
    }
    DEBUG("tx prepared\n");
    // or in tx_prepare?
   
    RAIL_RfIdleExt(RAIL_IDLE, true);
    DEBUG("RF changed to IDLE\n");
    
  //  RAIL_CalValues_t calValues;
  //   calValues.imageRejection = RAIL_CAL_INVALID_VALUE;
  //  RAIL_CalStart(&calValues, RAIL_CAL_ALL_PENDING, true);
    
  //  RAIL_RfIdleExt(RAIL_IDLE, true);
  //  RAIL_RfIdle();
  //  DEBUG("RF changed to IDLE\n");
    
    uint8_t r_ret = RAIL_TxDataLoad(&txData);
    
    if (r_ret != RAIL_STATUS_NO_ERROR) {
        // error
        LOG_ERROR("Can't load data to tx frame\n");
        return -1;
    }
    DEBUG("TX Data loaded\n");
    
   
    (void)txOption;
    (void)csmaConfig;
    (void)txData;
    
  
   // r_ret =  RAIL_TxStart(dev->channel, NULL, NULL);
    
    
    r_ret = RAIL_TxStartWithOptions(0, //dev->netdev.chan, // channel to transmit on
                                    &txOption,      // option for transmit
                                    RAIL_CcaCsma,   // function to call before transmitting -> use csma
                                    (void*) &csmaConfig);    // csma std config for 802.15.4
    
    if (r_ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can't start transmit error code %u \n", r_ret);
        return -1;
    }
    DEBUG("Started transmit\n");
    //
  //  xtimer_ticks32_t tt = {100000};
  //  xtimer_spin(tt);
   // DEBUG("sleep transmit\n");
    
    // TODO symetric blocking call. should be stated by callback
    while (RAIL_RfStateGet() == RAIL_RF_STATE_TX);
  //  CORE_EXIT_CRITICAL();
 //   ps();
    // set state
  //  dev->state = RAIL_TRANSCEIVER_STATE_TX;
    
    // 
    
    
    return (int) len-1;
}

static int _recv(netdev2_t* netdev, void* buf, size_t len, void* info)
{
    DEBUG("rail_netdev->recv called\n");
    return 0;
}

static void _isr(netdev2_t* netdev) 
{
   DEBUG("rail_netdev->isr called\n");
}
static int _get(netdev2_t* netdev, netopt_t opt, void* val, size_t max_len) 
{
    //DEBUG("rail_netdev->get called opt %s \n", netopt2str(opt));
    
    if (netdev == NULL) {
        return -ENODEV;
    }
    
    rail_t* dev = (rail_t*) netdev;
    (void) dev;
    
    // todo necessary to differencate if transceiver is acitive or not?
    int ret = -ENOTSUP;
    
    switch (opt) {
        case (NETOPT_PROTO): // layer 2
            DEBUG("get NETOPT_PROTO\n");
            break;
        case (NETOPT_IPV6_IID): // layer 2
            DEBUG("get NETOPT_IPV6_IID\n");
            break;
        case (NETOPT_MAX_PACKET_SIZE):
            assert(max_len >= sizeof(int16_t));
            *((uint16_t *)val) = IEEE802154_FRAME_LEN_MAX;
            ret = sizeof(uint16_t);
            break;
        case (NETOPT_IS_WIRED):
            // no wire attached ...
            ret = 0;
            break;
        case (NETOPT_ADDRESS):
            DEBUG("get NETOPT_ADDRESS\n");
            break;
        case (NETOPT_CHANNEL):
            DEBUG("get NETOPT_CHANNEL\n");
            break;
        case (NETOPT_CHANNEL_PAGE): // todo check if there is really nothing like a channel page?
            return -ENOTSUP;
            break;
        case (NETOPT_STATE):        // 
            assert(max_len >= sizeof(netopt_state_t));
            *((netopt_state_t *)val) = _get_state(dev);
            return sizeof(netopt_state_t);
            break;
        case (NETOPT_NID):
            DEBUG("get NETOPT_NID\n");
            break;
        case (NETOPT_ADDRESS_LONG):
            DEBUG("get NETOPT_ADDRESS_LONG\n");
            break;
        case (NETOPT_TX_POWER):     //
            assert(max_len >= sizeof(int16_t));
            // rail tx dbm has a factor of 10 -> loosing resolution here
            int32_t dbm = RAIL_TxPowerGet();
            dbm = dbm / PA_SCALING_FACTOR;
            *((uint16_t *)val) = (uint16_t) dbm;
            ret = sizeof(uint16_t);
            break;
        case (NETOPT_RETRANS):      // todo
            DEBUG("get NETOPT_RETRANS\n");
        
            break;
        case (NETOPT_PROMISCUOUSMODE): 
            if (dev->promiscuousMode == true) {
                *((netopt_enable_t *)val) = NETOPT_ENABLE;
            }
            else {
                *((netopt_enable_t *)val) = NETOPT_DISABLE;
            }
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_AUTOACK):      
            if (RAIL_AutoAckIsEnabled() == true) {
                *((netopt_enable_t *)val) =  NETOPT_ENABLE;
            } else {
                *((netopt_enable_t *)val) =  NETOPT_DISABLE;
            }
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_ACK_REQ):
            DEBUG("get NETOPT_ACK_REQ\n");
            break;
        case (NETOPT_PRELOADING):   // todo what does it do??
            ret = -ENOTSUP;
            break;
        case (NETOPT_RAWMODE):
            DEBUG("get NETOPT_RAWMODE\n");
            break;
        case (NETOPT_CSMA):         // todo can it be switched on / off?
            *((netopt_enable_t *)val) = NETOPT_ENABLE;
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CSMA_RETRIES):
            DEBUG("get NETOPT_CSMA_RETRIES\n");
            break;
        case (NETOPT_AUTOCCA):
            DEBUG("get NETOPT_AUTOCCA\n");
            break;
        case (NETOPT_SRC_LEN):
            DEBUG("get NETOPT_SRC_LEN\n");
            break;
        default:
            DEBUG("not supported netopt code at rail drv %d str %s \n", opt, netopt2str(opt));
            break;
    }
    if (ret != -ENOTSUP) {
        return ret;
    }
    // delegate cases to ieee802.15.4 layer
    //NETOPT_PROTO
    //
    
    ret = netdev2_ieee802154_get((netdev2_ieee802154_t *)netdev, opt, val, max_len);
    
    if (ret != -ENOTSUP) {
        return ret;
    }
    
    DEBUG("ieee802.15.4 could not handle netopt opt %s \n", netopt2str(opt));
    
    return ret;
    
}
static int _set(netdev2_t* netdev, netopt_t opt, void* val, size_t len)
{
    DEBUG("rail_netdev->set called opt %s \n", netopt2str(opt));
    // bei channel, testen ob channel zur frequenz passt
    return 0;
}


// impl local helper functions

netopt_state_t _get_state(rail_t* dev)
{
    
    // check state that can not 
    switch (dev->state) {
        case (RAIL_TRANSCEIVER_STATE_UNINITIALISIED):
            return NETOPT_STATE_OFF;
        case (RAIL_TRANSCEIVER_STATE_SLEEP):
            return NETOPT_STATE_SLEEP;
        case (RAIL_TRANSCEIVER_STATE_OFF):
            return NETOPT_STATE_OFF;
    }
    
    // check state from rail
    
    RAIL_RadioState_t state = RAIL_RfStateGet();
    
    switch (state) {
        case (RAIL_RF_STATE_IDLE):
            return NETOPT_STATE_IDLE;
        case (RAIL_RF_STATE_RX):
            return NETOPT_STATE_RX;
        case (RAIL_RF_STATE_TX):
            return NETOPT_STATE_TX;
    }
    
    return NETOPT_STATE_IDLE;
}
