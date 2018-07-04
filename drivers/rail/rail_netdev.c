/*
 * Copyright (C) 2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

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

#include "log.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#include "rail.h"
#include "ieee802154/rail_ieee802154.h"

#define _MAX_MHR_OVERHEAD   (25)

/* local declaration of driver methodes */
static int _send(netdev_t *netdev, const iolist_t *iolist);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);

/* buffer for outgoing frame */
static uint8_t frame[IEEE802154_FRAME_LEN_MAX + 1];


/* local helper functions */
static netopt_state_t _get_state(rail_t *dev);
static int _set_state(rail_t *dev, netopt_state_t state);

const netdev_driver_t rail_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};


static inline int rail_map_rail_status2errno(RAIL_Status_t code)
{
    switch (code) {
        case (RAIL_STATUS_NO_ERROR):
            return 0;
            break;
        case (RAIL_STATUS_INVALID_PARAMETER):
            return -EINVAL;
            break;
        case (RAIL_STATUS_INVALID_STATE):
            return -EPERM;
            break;
        case (RAIL_STATUS_INVALID_CALL):
            return -EOPNOTSUPP;
            break;
        case (RAIL_STATUS_SUSPENDED):
            break;
        default:
            break;
    }
}

static int _init(netdev_t *netdev)
{

    rail_t *dev = (rail_t *)netdev;

    DEBUG("rail_netdev->init called\n");

    /* set default channel */
#if (RAIL_RADIO_BAND == 2400)
    dev->netdev.chan = IEEE802154_DEFAULT_CHANNEL;
#elif (RAIL_RADIO_BAND == 868)
    dev->netdev.chan = 0; /* for 868MHz there is only one channel */
#elif (RAIL_RADIO_BAND == 915)
    dev->netdev.chan = 1;
#endif

#ifdef MODULE_GNRC_SIXLOWPAN
    dev->netdev.proto = GNRC_NETTYPE_SIXLOWPAN;
#elif MODULE_GNRC
    dev->netdev.proto = GNRC_NETTYPE_UNDEF;
#endif


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

    /* TODO state of the driver */

    return 0;
}

static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    DEBUG("rail_netdev->send called\n");

    rail_t *dev = (rail_t *)netdev;

    /*
       TODO check current state, make it depend what to do
       if tx, return error
       if init or so return error
       if calibaration error
       if idle or rx, send
        if waiting for ack
          no timeout return error
          timeout update stat and continue
     */

    /* prepare frame, cpy header and payload */
    size_t len = 1; /* start with 1, first byte have to be the length */
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {
        /* current packet data + FCS too long */
        if ((len + iol->iol_len + 2) > IEEE802154_FRAME_LEN_MAX) {
            DEBUG("[rail] error: packet too large (len so far %u byte, combined %u) to be send\n",
                  (unsigned)len + 2, (unsigned) len + 2 + iol->iol_len);
            return -EOVERFLOW;
        }
#ifdef MODULE_NETSTATS_L2
        netdev->stats.tx_bytes += len;
#endif
        memcpy(frame + len, iol->iol_base, iol->iol_len);
        len += iol->iol_len;
    }

    int ret = rail_transmit_frame(dev, frame, len);

    if (ret != 0) {
        LOG_ERROR("Can not send data\n");
        return ret;
    }

    return (int)len - 1;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{

    DEBUG("rail_netdev->recv called\n");

    rail_t *dev = (rail_t *)netdev;

    RAIL_RxPacketHandle_t pack_handle;
    RAIL_RxPacketInfo_t pack_info;
    RAIL_RxPacketDetails_t pack_details;
    RAIL_Status_t ret;

    /*
       the "receiving of a packet" becomes a bit ugly, since the riot driver api is
       optimized for low level hardware interaction. Here the silabs driver blob
       already did the whole low level hw interaction. By the time this methode is
       called the whole packet is already read and stored in memory.
       Normally the packet have to be processed within the registered handler of
       the driver blob. It is possible to hold a packet, to process it later, as in
       in this methode.
       With RAIL_RX_PACKET_HANDLE_OLDEST a handle to the oldest not processed
       packet is accessable, but because the _recv methode is called several times
       by the upper protocol layer it is possible this reference changes. Therefore
       it have to be stored, for the next call of _recv().
       After processing the packet it has to be released with
       RAIL_ReleaseRxPacket().

       TODO
       - What if there are more packets waiting? How to loop through them?
       - What if this method is called, because a new packet arrived and the stack
       is still busy processing the first?
       - Or worst, if it causes a race condition?
       - What if for whatever reason the upper layer does only call this methode
       once for a packet? Than the dev->lastRxPacketHandle would not be reset
       and the next call for a suppost new packet would yield the packet before.
     */

    /* if this is the first call for a new packet, the handle is (have to be)
       invalid */
    if (dev->lastRxPacketHandle == RAIL_RX_PACKET_HANDLE_INVALID) {
        /* get the oldest not yet processed packet */
        pack_handle = RAIL_RX_PACKET_HANDLE_OLDEST;
    }
    else {
        /* otherwise this is the second, third ... call
           we use the saved handle form the call before */

        pack_handle = dev->lastRxPacketHandle;
    }

    /* first packet info -> payload length and the handle of the packet
       if pack_handle is RAIL_RX_PACKET_HANDLE_OLDEST, we get the oldest not
       processed packet, otherwise the function returns the same handle as given
     */
    pack_handle = RAIL_GetRxPacketInfo(dev->rhandle,
                                       pack_handle,
                                       &pack_info);
    dev->lastRxPacketHandle = pack_handle;

    /* buf == NULL && len == 0 -> return packet size, no dropping */
    if (buf == NULL && len == 0) {
        DEBUG("_recv: no dropping return packet size: 0x%02x\n", pack_info.packetBytes);
        /* -1 because only payload length, without the packet length byte */
        return pack_info.packetBytes - 1;
    }
    /* buf == NULL && len > 0 -> return packet size + drop it */
    if (buf == NULL && len > 0) {
        /* drop it */
        DEBUG("_recv: drop packet - return packet size: 0x%02x\n", pack_info.packetBytes);
        RAIL_ReleaseRxPacket(dev->rhandle, pack_handle);
        dev->lastRxPacketHandle = RAIL_RX_PACKET_HANDLE_INVALID;
        /* -1 because only payload length, without the packet length byte */
        return pack_info.packetBytes;
    }

    /* hurray, we are finally at the stage to move the payload to the upper
       layer
     */


    /* clear info struct */
    memset(&pack_details, 0, sizeof(RAIL_RxPacketDetails_t));

    /* get more infos about the packet */
    ret = RAIL_GetRxPacketDetails(dev->rhandle, pack_handle, &pack_details);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Error receiving new packet / frame - msg %s\n", rail_error2str(ret));
        return -1;
    }

    DEBUG("time received: %lu "
          "crcStatus %s "
          "isAck: %s "
          "subPhy: %u "
          "rssiLatch: %d dBm "
          "lqi: %u "
          "syncWordId: %u "
          "antenna id: %u "
          "payload size: %u \n",
          pack_details.timeReceived.packetTime,
          pack_details.crcPassed ? "Passed" : "Failed",
          pack_details.isAck ? "Ack" : "Not a Ack",
          pack_details.subPhyId,
          pack_details.rssi,
          pack_details.lqi,
          pack_details.syncWordId,
          pack_details.antennaId,
          pack_info.packetBytes);

#ifdef MODULE_NETSTATS_L2
    netdev->stats.rx_count++;
    netdev->stats.rx_bytes += pack_info.packetBytes;
#endif

    /* TODO question: with length info in byte 0 or without? */
    /*  - first try without, skip it (seams to work) */
    pack_info.firstPortionData++;
    pack_info.firstPortionBytes--;
    pack_info.packetBytes--;

    /* copy payload from packet to the provided buffer */
    RAIL_CopyRxPacket((uint8_t *)buf, &pack_info);



    /*
       DEBUG("Print buf cpy size %d: ", cpy_size);
       for (int i = 0; i < cpy_size; i++) {
        if (i % 4 == 0) DEBUG("\n");
        DEBUG("0x%02x ", ((uint8_t*)buf)[i]);
       }
       DEBUG("\n");
     */

    /* if requested, provide meta infos as well */
    /* TODO move  RAIL_GetRxPacketDetails in this block as well? */
    if (info != NULL) {
        netdev_ieee802154_rx_info_t *rx_info = info;
        rx_info->rssi = pack_details.rssi;
        rx_info->lqi = pack_details.lqi;
    }

    /* free packet, set handle to null */
    RAIL_ReleaseRxPacket(dev->rhandle, pack_handle);
    dev->lastRxPacketHandle = RAIL_RX_PACKET_HANDLE_INVALID;

    return pack_info.packetBytes;
}

static void _isr(netdev_t *netdev)
{
    DEBUG("rail_netdev->isr called\n");
    /* there is not much we can do here, because what is normally is done here,
       is allready done by the driver blob and the _rail_radio_event_handler.
     */
    netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
}
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len)
{

    if (netdev == NULL) {
        return -ENODEV;
    }

    rail_t *dev = (rail_t *)netdev;

    /* TODO
        - is it necessary to differencate if transceiver is active or not?
        - What is a channel page and how to get this info out of the RAIL API?
        - NETOPT_RETRANS
        - Can CSMA be switched on / off and if yes how?
        - NETOPT_BANDWIDTH could be calculated, but is it usefull?
        - NETOPT_CHANNEL_FREQUENCY
        - NETOPT_AUTOCCA
     */
    int ret = -ENOTSUP;

    switch (opt) {
        case (NETOPT_MAX_PACKET_SIZE):
            assert(max_len >= sizeof(int16_t));
            *((uint16_t *)val) = IEEE802154_FRAME_LEN_MAX - _MAX_MHR_OVERHEAD;
            ret = sizeof(uint16_t);
            break;
        case (NETOPT_CHANNEL_PAGE):
            /* TODO */
            break;
        case (NETOPT_STATE):
            assert(max_len >= sizeof(netopt_state_t));
            *((netopt_state_t *)val) = _get_state(dev);
            return sizeof(netopt_state_t);
            break;
        case (NETOPT_TX_POWER):
            assert(max_len >= sizeof(int16_t));
            /* rail tx dbm has a factor of 10 -> loosing resolution here */
            /* get transmitt power with new RAIL helper function */
            RAIL_TxPower_t power_tx_ddBm = RAIL_GetTxPowerDbm(dev->rhandle);
            *((uint16_t *)val) = (int16_t) (power_tx_ddBm / 10);
            ret = sizeof(int16_t);
            break;
        case (NETOPT_RETRANS):
            /* TODO */
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
                *((netopt_enable_t *)val) = NETOPT_ENABLE;
            }
            else {
                *((netopt_enable_t *)val) = NETOPT_DISABLE;
            }
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CSMA):
            /* if tries == 0 -> CSMA is disabled, or rather the packet is send
               immediately
             */
            if (dev->csma_config.csmaTries == 0) {
                *((netopt_enable_t *)val) = NETOPT_DISABLE;
            }
            else {
                *((netopt_enable_t *)val) = NETOPT_ENABLE;
            }
            ret = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CSMA_RETRIES):
            assert(max_len >= sizeof(int8_t));
            *((uint8_t *)val) = dev->csma_config.csmaTries;
            ret = sizeof(uint8_t);
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
        default:
            /* DEBUG("not supported netopt code at rail drv %d str %s \n", opt,
               netopt2str(opt)); */
            break;
    }
    if (ret != -ENOTSUP) {
        return ret;
    }

    ret = netdev_ieee802154_get((netdev_ieee802154_t *)netdev, opt, val, max_len);

    if (ret != -ENOTSUP) {
        return ret;
    }

    /*DEBUG("ieee802.15.4 could not handle netopt opt %s \n", netopt2str(opt));*/

    return ret;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len)
{


    if (netdev == NULL) {
        return -ENODEV;
    }

    rail_t *dev = (rail_t *)netdev;
    int res = -ENOTSUP;
    RAIL_Status_t rail_ret;
    uint16_t le_u16;
    uint64_t le_u64;

    /* TODO wake up transceiver? necessary? or done automaticaly by RAIL driver
       blob?
     */

    switch (opt) {

        case (NETOPT_CHANNEL):
            /* since we have to provide the channel for each tx or rx, just
               change the attribute in the netdev struct */
            assert(len == sizeof(uint16_t));
            uint8_t chan = (((const uint16_t *)val)[0]) & UINT8_MAX;

            if (dev->params.freq == RAIL_TRANSCEIVER_FREQUENCY_2P4GHZ) {
                if (chan < RAIL_2P4GH_MIN_CHANNEL || chan > RAIL_2P4GH_MAX_CHANNEL) {
                    res = -EINVAL;
                    break;
                }
            }
            else if (dev->params.freq == RAIL_TRANSCEIVER_FREQUENCY_868MHZ) {
                /* 868MHz has only one channel, channel 0! */
                if (chan != RAIL_868MHZ_DEFAULT_CHANNEL) {
                    res = -EINVAL;
                    break;
                }
            }
            else if (dev->params.freq == RAIL_TRANSCEIVER_FREQUENCY_912MHZ) {
                if (chan > RAIL_912MHZ_MAX_CHANNEL) {
                    res = -EINVAL;
                    break;
                }
            }
            else {
                res = -EINVAL;
                LOG_ERROR("Unknown radio frequency configured\n");
                assert(false);
                break;
            }
            /* since we have to provide the channel for each tx or rx, just
               change the attribute in the netdev struct */
            /* TODO if chan_old != chan_new -> interupt rx op? */
            /* don't set res to set netdev_ieee802154_t::chan */
            dev->netdev.chan = chan;

            break;
        case (NETOPT_CHANNEL_PAGE):
            /* TODO?? */
            break;
        case (NETOPT_ADDRESS):
            assert(len <= sizeof(uint16_t));
            /* RIOT uses Big endian, RAIL driver blog little ... */

            le_u16 = ntohs(*((const uint16_t *)val));

            /* RAIL driver blob can manage upto RAIL_IEEE802154_MAX_ADDRESSES
               TODO how does RIOT handle multible short addresses?
               atm just at pos 0
             */
            rail_ret = RAIL_IEEE802154_SetShortAddress(dev->rhandle, le_u16, 0);

            if (rail_ret != RAIL_STATUS_NO_ERROR) {
                LOG_ERROR("[rail] error setting short address: msg: %s\n",
                          rail_error2str(rail_ret));
                res = -EFAULT;
                break;
            }

            /* don't set res to set netdev_ieee802154_t::short_addr */
            break;
        case (NETOPT_ADDRESS_LONG):
            assert(len <= sizeof(uint64_t));
            /* RAIL driver blob can manage upto RAIL_IEEE802154_MAX_ADDRESSES
               TODO how does RIOT handle multible long addresses?
               atm just at pos 0
             */
            /* now the long addr ... the RAIL API docu says it have to be in
               "over the air byte order", therefore little endian again ... */
            le_u64 = byteorder_swapll(*((const uint64_t *)val));


            rail_ret = RAIL_IEEE802154_SetLongAddress(dev->rhandle, (uint8_t *)&le_u64, 0);

            if (rail_ret != RAIL_STATUS_NO_ERROR) {
                LOG_ERROR("[rail] error setting long address: msg: %s\n",
                          rail_error2str(rail_ret));
                res = -EFAULT;
                break;
            }
            /* don't set res to set netdev_ieee802154_t::long_addr */
            break;
        case (NETOPT_NID):
            assert(len <= sizeof(uint16_t));
            /* RIOT driver blob supports multible PAN IDs. Does RIOT as well? */

            rail_ret = RAIL_IEEE802154_SetPanId(dev->rhandle, *((const uint16_t *)val), 0);

            if (rail_ret != RAIL_STATUS_NO_ERROR) {
                LOG_ERROR("[rail] error setting NIB/pan id: msg: %s\n",
                          rail_error2str(rail_ret));
                res = -EFAULT;
                break;
            }

            /* don't set res to set netdev_ieee802154_t::pan */
            break;
        case (NETOPT_TX_POWER):
            assert(len <= sizeof(int16_t));

            /* RAIL driver blob supports deci-dBm, RIOT only dBm*/
            int16_t dBm = *((const int16_t *)val);
            int16_t ddBm = dBm * 10;

            rail_ret = RAIL_SetTxPowerDbm(dev->rhandle, ddBm);

            if (rail_ret != RAIL_STATUS_NO_ERROR) {
                LOG_ERROR("[rail] error setting NIB/pan id: msg: %s\n",
                          rail_error2str(rail_ret));
                res = -EFAULT;
                break;
            }
            res = sizeof(uint16_t);
            break;
        case (NETOPT_STATE):
            assert(len <= sizeof(netopt_state_t));
            res = _set_state(dev, *((const netopt_state_t *)val));
            break;
        case (NETOPT_AUTOACK):
            break;
        case (NETOPT_RETRANS):
            break;
        case (NETOPT_PROMISCUOUSMODE):

            /* we have to store this info, because we can not ask the RAIL
               driver blob, if promiscuousMode is set
             */
            dev->promiscuousMode = ((const bool *)val)[0];

            rail_ret = RAIL_IEEE802154_SetPromiscuousMode(dev->rhandle, dev->promiscuousMode);

            if (rail_ret != RAIL_STATUS_NO_ERROR) {
                LOG_ERROR("[rail] error setting promiscuous mode: msg: %s\n",
                          rail_error2str(rail_ret));
                res = -EFAULT;
                break;
            }
            res = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CSMA):
            /* deactivate CSMA */
            if (((const bool *)val)[0] == false) {
                dev->csma_config.csmaTries = 0;
            }
            else {
                // set it to the default value?
                dev->csma_config.csmaTries = RAIL_DEFAULT_CSMA_TRIES;
            }
            res = sizeof(netopt_enable_t);
            break;
        case (NETOPT_CSMA_RETRIES):
            assert(len <= sizeof(uint8_t));
            /* tries == 0 -> CSMA is disabled */
            if (dev->csma_config.csmaTries == 0 ||
                (*((uint8_t *)val) > RAIL_MAX_LBT_TRIES)) {
                /* If CSMA is disabled, don't allow setting retries */
                res = -EINVAL;
            }
            else {
                dev->csma_config.csmaTries = *((const uint8_t *)val);
                res = sizeof(uint8_t);
            }
            break;
        case (NETOPT_CCA_THRESHOLD):
            assert(len <= sizeof(int8_t));

            rail_ret = RAIL_SetCcaThreshold(dev->rhandle, *((const int8_t *)val));

            if (rail_ret != RAIL_STATUS_NO_ERROR) {
                LOG_ERROR("[rail] error CCA threshold: msg: %s\n",
                          rail_error2str(rail_ret));
                res = -EFAULT;
                break;
            }

            res = sizeof(int8_t);
            break;
        default:
            break;
    }

    if (res == -ENOTSUP) {
        res = netdev_ieee802154_set((netdev_ieee802154_t *)netdev, opt, val, len);
    }

    return res;

    /* TODO

        - NETOPT_CHANNEL_PAGE // how? what is it? relevant?
        - NETOPT_AUTOACK
        - NETOPT_RETRANS

        - bool     RAIL_IEEE802154_IsEnabled (void) ?
        - void  RAIL_EnableTxHoldOff (RAIL_Handle_t railHandle, bool enable)
        - bool  RAIL_IsTxHoldOffEnabled (RAIL_Handle_t railHandle)

        - No Option for pan coord? RAIL_Status_t
              RAIL_IEEE802154_SetPanCoordinator (bool isPanCoordinator)
        - NETOPT_IPV6_ADDR_REMOVE
            -  Set to 0x00 00 00 00 00 00 00 00 to disable for this index.


     */

    DEBUG("rail_netdev->set called opt %s val %p len %d \n", netopt2str(opt), val, len);

    return 0;
}

/* impl local helper functions */

netopt_state_t _get_state(rail_t *dev)
{

    /* check state that can not */
    switch (dev->state) {
        case (RAIL_TRANSCEIVER_STATE_UNINITIALIZED):
            return NETOPT_STATE_OFF;
        case (RAIL_TRANSCEIVER_STATE_SLEEP):
            return NETOPT_STATE_SLEEP;
        case (RAIL_TRANSCEIVER_STATE_OFF):
            return NETOPT_STATE_OFF;
    }

    /* check state from RAIL */

    RAIL_RadioState_t state = RAIL_GetRadioState(dev->rhandle);

    if (state & RAIL_RF_STATE_RX) {
        return NETOPT_STATE_RX;
    }
    else if (state & RAIL_RF_STATE_TX) {
        return NETOPT_STATE_TX;
    }
    else if (state == RAIL_RF_STATE_IDLE) {
        return NETOPT_STATE_IDLE;
    }

    return NETOPT_STATE_IDLE;
}

static int _set_state(rail_t *dev, netopt_state_t state)
{
    (void) dev;
    switch (state) {
        case NETOPT_STATE_STANDBY:

            break;
        case NETOPT_STATE_SLEEP:

            break;
        case NETOPT_STATE_IDLE:

            break;
        case NETOPT_STATE_TX:

            break;
        case NETOPT_STATE_RESET:
            break;
        default:
            return -ENOTSUP;
    }
    return -ENOTSUP;
    /*return sizeof(netopt_state_t);*/
}
