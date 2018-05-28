
#include <string.h>

#include "pa.h"
#include "pti.h"
#include "rail.h"
#include "em_core.h"

#include "iolist.h"

#include "net/netdev.h"
#include "net/netopt.h"

#include "ps.h"
#include "xtimer.h"

#include "errno.h"

#include "radio.h"

#include "rail_drv.h"
#include "rail_netdev.h"
#include "rail_internal.h"
#include "rail_registers.h"



#include "log.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

// local declaration of driver methodes
static int _send(netdev_t* netdev, const iolist_t *iolist);
static int _recv(netdev_t* netdev, void* buf, size_t len, void* info);
static int _init(netdev_t* netdev);
static void _isr(netdev_t* netdev);
static int _get(netdev_t* netdev, netopt_t opt, void* val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);


static  uint8_t frame[IEEE802154_FRAME_LEN_MAX + 1];


static const RAIL_CsmaConfig_t csma_config = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

// local helper functions
netopt_state_t _get_state(rail_t* dev);

const netdev_driver_t rail_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};


static int _init(netdev_t* netdev) {

    rail_t* dev = (rail_t*) netdev;


    DEBUG("rail_netdev->init called\n");
     // set default channel
#if (RAIL_RADIO_BAND == 2400)
    dev->netdev.chan = IEEE802154_DEFAULT_CHANNEL;
#elif (RAIL_RADIO_BAND == 868)
    dev->netdev.chan = 0;
#elif (RAIL_RADIO_BAND == 915)
    dev->netdev.chan = 1;
#endif

#ifdef MODULE_GNRC_SIXLOWPAN
    dev->netdev.proto = GNRC_NETTYPE_SIXLOWPAN;
#elif MODULE_GNRC
    dev->netdev.proto = GNRC_NETTYPE_UNDEF;
#endif

    // do not start in promiscuousMode
    dev->promiscuousMode = false;
    dev->macNoSleep = false;

    netdev->driver = &rail_driver;

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


static int _send(netdev_t* netdev, const iolist_t *iolist) {
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
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {
        /* current packet data + FCS too long */
        if ((len + iol->iol_len + 2) > IEEE802154_FRAME_LEN_MAX) {
            DEBUG("[rail] error: packet too large (%u byte) to be send\n",
                  (unsigned)len + 2);
            return -EOVERFLOW;
        }
#ifdef MODULE_NETSTATS_L2
        netdev->stats.tx_bytes += len;
#endif
        memcpy(frame+len, iol->iol_base, iol->iol_len);
        len += iol->iol_len;

    }

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

   int ret = rail_transmit_frame(dev, frame, len);

   if (ret != 0) {
       LOG_ERROR("Can not send data\n");
       return ret;
   }

    return (int) len-1;
}

static int _recv(netdev_t* netdev, void* buf, size_t len, void* info) {

    DEBUG("rail_netdev->recv called\n");

    rail_t* dev = (rail_t*) netdev;

    RAIL_RxPacketHandle_t   pack_handle;
    RAIL_RxPacketInfo_t     pack_info;
    RAIL_RxPacketDetails_t  pack_details;
    RAIL_Status_t           ret;

    // get infos about the received package
    // TODO either store the packethandle or deceide if oldest or newest is the
    // right package ...
    // here dev->lastRxPacketHandle is only set in the _recv function, because
    // upper stack layer are calling this function several times for one receive
    // ...

    if (dev->lastRxPacketHandle == RAIL_RX_PACKET_HANDLE_INVALID ) {
        // first call for a new packet
        // RAIL_RX_PACKET_HANDLE_OLDEST
        // RAIL_RX_PACKET_HANDLE_NEWEST
        pack_handle = RAIL_RX_PACKET_HANDLE_OLDEST;
    } else {
        // second call, use saved handle
        pack_handle = dev->lastRxPacketHandle;
    }

    // first packet info -> payload length
    pack_handle = RAIL_GetRxPacketInfo(  dev->rhandle,
                                        pack_handle,
                                        &pack_info
                                     );
    dev->lastRxPacketHandle = pack_handle;

    // buf == NULL && len == 0 -> return packet size, no dropping
    if (buf == NULL && len == 0) {
        DEBUG("_recv: no dropping return packet size: 0x%02x\n", pack_info.packetBytes);
        // -1 because no len infos
        return pack_info.packetBytes -1;
    }
    // buf == NULL && len > 0 -> return packet size + drop it
    if (buf == NULL && len > 0) {
        // drop it
        DEBUG("_recv: drop packet - return packet size: 0x%02x\n", pack_info.packetBytes);
        RAIL_ReleaseRxPacket(dev->rhandle, pack_handle);
        dev->lastRxPacketHandle = RAIL_RX_PACKET_HANDLE_INVALID;
        // -1 because no len infos
        return pack_info.packetBytes;
    }

    // clear info struct
    memset(&pack_details, 0, sizeof(RAIL_RxPacketDetails_t));

    // get more infos about the packet
    ret = RAIL_GetRxPacketDetails(dev->rhandle, pack_handle, &pack_details);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Error receiving new packet / frame - msg %s\n", rail_error2str(ret));
        return -1;
    }

    DEBUG("time received: %lu\n"
            "crcStatus %s \n"
            "isAck: %s\n"
            "subPhy: %u\n"
            "rssiLatch: %d dBm\n"
            "lqi: %u\n"
            "syncWordId: %u\n"
            "antenna id: %u\n"
            "payload size: %u\n",
            pack_details.timeReceived.packetTime,
            pack_details.crcPassed ? "Passed":"Failed",
            pack_details.isAck ? "Ack" : "Not a Ack",
            pack_details.subPhyId,
            pack_details.rssi,
            pack_details.lqi ,
            pack_details.syncWordId,
            pack_details.antennaId,
            pack_info.packetBytes
         );

    // question: with length info in byte 0 or without?
    // first try without, skip it
    pack_info.firstPortionData++;
    pack_info.firstPortionBytes--;
    pack_info.packetBytes--;

    // copy payload of packet
    RAIL_CopyRxPacket((uint8_t*)buf, &pack_info);

    /*
    DEBUG("Print buf cpy size %d: ", cpy_size);
    for (int i = 0; i < cpy_size; i++) {
        if (i % 4 == 0) DEBUG("\n");
        DEBUG("0x%02x ", ((uint8_t*)buf)[i]);
    }
    DEBUG("\n");
*/
    if (info != NULL) {
        netdev_ieee802154_rx_info_t* rx_info = info;
        rx_info->rssi = pack_details.rssi;
        rx_info->lqi = pack_details.lqi;
    }

    // free packet, set handle to null
    RAIL_ReleaseRxPacket(dev->rhandle, pack_handle);
    dev->lastRxPacketHandle = RAIL_RX_PACKET_HANDLE_INVALID;

    return pack_info.packetBytes;
}

static void _isr(netdev_t* netdev) {
   DEBUG("rail_netdev->isr called\n");
   // dunno what to do, but call the callback ...
   netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
}
static int _get(netdev_t* netdev, netopt_t opt, void* val, size_t max_len) {
    //DEBUG("rail_netdev->get called opt %s \n", netopt2str(opt));

    if (netdev == NULL) {
        return -ENODEV;
    }

    rail_t* dev = (rail_t*) netdev;
    (void) dev;

    // todo necessary to differencate if transceiver is acitive or not?
    int ret = -ENOTSUP;

    switch (opt) {
        case (NETOPT_MAX_PACKET_SIZE):
            assert(max_len >= sizeof(int16_t));
            *((uint16_t *)val) = IEEE802154_FRAME_LEN_MAX;
            ret = sizeof(uint16_t);
            break;
        case (NETOPT_IS_WIRED):
            // no wire attached ...
            ret = 0;
            break;
        case (NETOPT_CHANNEL_PAGE): // todo check if there is really nothing like a channel page?
            break;
        case (NETOPT_STATE):
            assert(max_len >= sizeof(netopt_state_t));
            *((netopt_state_t *)val) = _get_state(dev);
            return sizeof(netopt_state_t);
            break;
        case (NETOPT_TX_POWER):
            assert(max_len >= sizeof(int16_t));
            // rail tx dbm has a factor of 10 -> loosing resolution here
            // get transmitt power
            RAIL_TxPowerLevel_t power_level_tx = RAIL_GetTxPower(dev->rhandle);
            RAIL_TxPower_t power_tx_ddBm = RAIL_ConvertRawToDbm(dev->rhandle,
#if RAIL_RADIO_BAND == 2400
                            RAIL_TX_POWER_MODE_2P4_HP,      // 2.4GHZ HighPower, TODO low power?
#elif (RAIL_RADIO_BAND == 868) || (RAIL_RADIO_BAND == 915)
                            RAIL_TX_POWER_MODE_SUBGIG,
#endif
                            power_level_tx
                            );
            *((uint16_t *)val) = (uint16_t) power_tx_ddBm / 10;
            ret = sizeof(uint16_t);
            break;
        case (NETOPT_RETRANS):      // todo
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
            if (RAIL_IsAutoAckEnabled(dev->rhandle) == true) {
                *((netopt_enable_t *)val) =  NETOPT_ENABLE;
            } else {
                *((netopt_enable_t *)val) =  NETOPT_DISABLE;
            }
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_PRELOADING):   // todo what does it do??
            break;
        case (NETOPT_RAWMODE):
            break;
        case (NETOPT_CSMA):         // todo can it be switched on / off?
            *((netopt_enable_t *)val) = NETOPT_ENABLE;
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CSMA_RETRIES):
            assert(max_len >= sizeof(int8_t));
            *((uint8_t *)val) = csma_config.csmaTries;
            ret = sizeof(uint8_t);
            break;
        case (NETOPT_AUTOCCA):
            break;
        case (NETOPT_MAC_NO_SLEEP):
            if (dev->macNoSleep == false) {
                *((netopt_enable_t *)val) =  NETOPT_DISABLE;
            } else {
                *((netopt_enable_t *)val) =  NETOPT_ENABLE;
            }
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CHANNEL_HOP):
            break;
        case (NETOPT_CODING_RATE):
            break;

        case (NETOPT_BANDWIDTH):
            /*
            assert(max_len >= sizeof(int8_t));
            // bits/seconds
            uint32_t bw = RAIL_BitRateGet();
            uint8_t bw_kb = bw/1000;
            *((uint8_t *)val) = bw_kb;
            ret = sizeof(uint8_t);
            */
            break;
        case (NETOPT_CHANNEL_FREQUENCY):
            break;
        case (NETOPT_SINGLE_RECEIVE):
            break;
        case (NETOPT_IQ_INVERT):
            break;
        case (NETOPT_SRC_LEN):
            break;
        case (NETOPT_ADDRESS_LONG):
            break;
        case (NETOPT_DEVICE_TYPE):
            break;
        case (NETOPT_ADDRESS):
            break;
        case (NETOPT_CHANNEL):
            break;
        case (NETOPT_NID):
            break;
        case (NETOPT_SPREADING_FACTOR):
            break;
        case (NETOPT_ACK_REQ):
            break;
        case (NETOPT_STATS):
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

    ret = netdev_ieee802154_get((netdev_ieee802154_t *)netdev, opt, val, max_len);

    if (ret != -ENOTSUP) {
        return ret;
    }

 //   DEBUG("ieee802.15.4 could not handle netopt opt %s \n", netopt2str(opt));

    return ret;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len) {
    // make gcc happy
    netdev = netdev;
    len = len;
    DEBUG("rail_netdev->set called opt %s val %p \n", netopt2str(opt), val);
    // bei channel, testen ob channel zur frequenz passt
    // bool     RAIL_IEEE802154_IsEnabled (void)
    // bool     RAIL_IEEE802154_SetPanId (uint16_t panId)
    // bool     RAIL_IEEE802154_SetShortAddress (uint16_t shortAddr)
    // bool     RAIL_IEEE802154_SetLongAddress (uint8_t *longAddr)
    // RAIL_Status_t    RAIL_IEEE802154_SetPanCoordinator (bool
    // isPanCoordinator)
    // RAIL_Status_t    RAIL_IEEE802154_SetPromiscuousMode (bool enable)
    //
    return 0;
}


// impl local helper functions

netopt_state_t _get_state(rail_t* dev) {

    // check state that can not
    switch (dev->state) {
        case (RAIL_TRANSCEIVER_STATE_UNINITIALIZED):
            return NETOPT_STATE_OFF;
        case (RAIL_TRANSCEIVER_STATE_SLEEP):
            return NETOPT_STATE_SLEEP;
        case (RAIL_TRANSCEIVER_STATE_OFF):
            return NETOPT_STATE_OFF;
    }

    // check state from rail

    RAIL_RadioState_t state = RAIL_GetRadioState(dev->rhandle);

    if (state & RAIL_RF_STATE_RX) {
        return NETOPT_STATE_RX;
    } else if (state & RAIL_RF_STATE_TX) {
        return NETOPT_STATE_TX;
    } else if (state == RAIL_RF_STATE_IDLE){
        return NETOPT_STATE_IDLE;
    }

    return NETOPT_STATE_IDLE;
}
