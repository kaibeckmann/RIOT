/*
 * Copyright (C) 2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "byteorder.h"

#include "radio.h"

/* gecko sdk rail lib includes*/
#include "em_core.h"
#include "rail.h"
#include "rail_chip_specific.h"
#include "pa_conversions_efr32.h"
#include "pti.h"
#include "pa.h"
#include "rail_assert_error_codes.h"
#include "ieee802154/rail_ieee802154.h"

/* riot os rail driver includes*/
#include "rail_drv.h"
#include "rail_netdev.h"

#include "net/ieee802154.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#include "log.h"

/*  RADIO CONFIGURATION */
/* channel config for sub GHz radio
 */
#if (RAIL_RADIO_BAND == 868) || (RAIL_RADIO_BAND == 915)

#if (RAIL_RADIO_BAND == 868)
static RAIL_ChannelConfigEntryAttr_t radio_channel_entry_868;
#elif (RAIL_RADIO_BAND == 915)
static RAIL_ChannelConfigEntryAttr_t radio_channel_entry_915;
#endif

static const RAIL_ChannelConfigEntry_t radio_channel_entry[] = {
#if (RAIL_RADIO_BAND == 868)
    { .phyConfigDeltaAdd = NULL, /* Add this to default config for this entry */
      .baseFrequency = 868300000U,
      .channelSpacing = 600000U,
      .physicalChannelOffset = 0,
      .channelNumberStart = 0,
      .channelNumberEnd = 0,
      .maxPower = RAIL_TX_POWER_MAX,
      .attr = &radio_channel_entry_868 },
#elif (RAIL_RADIO_BAND == 915)
    { .phyConfigDeltaAdd = NULL, /* Add this to default config for this entry */
      .baseFrequency = 906000000U,
      .channelSpacing = 2000000U,
      .physicalChannelOffset = 1,
      .channelNumberStart = 1,
      .channelNumberEnd = 10,
      .maxPower = RAIL_TX_POWER_MAX,
      .attr = &radio_channel_entry_915 }
#endif
};

#endif

/* IEEE 802.15.4 config for 868MHz and 912MHz */
#if (RAIL_RADIO_BAND == 868)

static const uint32_t ieee802154_config_863[] = RAIL_IEEE802154_CONFIG_868MHZ;
static const uint32_t ieee802154_config_863_min[] = {
    0xFFFFFFFFUL,
};

static const RAIL_ChannelConfig_t _rail_radio_channel_config = {
    .phyConfigBase = ieee802154_config_863,
    .phyConfigDeltaSubtract = ieee802154_config_863_min,
    .configs = &radio_channel_entry[0],
    .length = 1
};
#elif (RAIL_RADIO_BAND == 915)

static const uint32_t ieee802154_config_863[] = RAIL_IEEE802154_CONFIG_915MHZ;
static const uint32_t ieee802154_config_915_min[] = {
    0xFFFFFFFFUL,
};

static const RAIL_ChannelConfig_t _rail_radio_channel_config = {
    .phyConfigBase = ieee802154_config_915,
    .phyConfigDeltaSubtract = ieee802154_config_915_min,
    .configs = &radio_channel_entry[0],
    .length = 1
};
#endif

static const RAIL_IEEE802154_Config_t _rail_ieee802154_config = {
    .addresses = NULL,
    .ackConfig = {
        .enable = true,                     /* Turn on auto ACK for IEEE 802.15.4 */
        .ackTimeout = 1200,                 /* (from mbed driver) why 1200? -> docu says 54 symbols * 16 us/symbol = 864 us */
        .rxTransitions = {
            .success = RAIL_RF_STATE_RX,    /* after rx -> state rx */
            .error = RAIL_RF_STATE_RX       /* ignored */
        },
        .txTransitions = {
            .success = RAIL_RF_STATE_RX,    /* after tx -> state rx */
            .error = RAIL_RF_STATE_RX       /* ignored */
        }
    },
    .timings = { .idleToRx = 100,
                 /* Make txToRx slightly lower than desired to make sure we get to
                    RX in time
                  */
                 .txToRx = 192 - 10,
                 .idleToTx = 100,
                 .rxToTx = 192,
                 .rxSearchTimeout = 0,
                 .txToRxSearchTimeout = 0 },
    .framesMask = RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
    .promiscuousMode = false,
    .isPanCoordinator = false
};

static const RAIL_CsmaConfig_t _rail_csma_default_config = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

/* TODO use define to restrict to supported freq */
RAIL_DECLARE_TX_POWER_VBAT_CURVES(piecewiseSegments, curvesSg, curves24Hp, curves24Lp);

/********************* BUFFER MANAGEMENT ***************************/

/* tx buffer */
static uint8_t _transmit_buffer[IEEE802154_FRAME_LEN_MAX + 1];

/********************* LOKAL VARIABLES ******************************/

/* ref to rail_t/ netdev_t struct for this driver
   TODO howto distinguish between multible netdevs?
   necessary for multiprotocol support, multible instances of the driver etc.
   Possible solution: map with an rhandle as key and netdev as value?
 */
static rail_t *_rail_dev = NULL;

/************************ private functions *********************************/

/* callback handler for RAIL driver blob, get called to handle events.
   The hw irqs are allready handled when called
 */
static void _rail_radio_event_handler(RAIL_Handle_t rhandle, RAIL_Events_t event);

/*
 * TODO docu
 *
 * TODO what shall be the difference between setup and init?
 */
void rail_setup(rail_t *dev, const rail_params_t *params)
{

    netdev_t *netdev = (netdev_t *)dev;

    /* register driver (defined in rail_netdev) */
    netdev->driver = &rail_driver;

    /* init dev with params */
    memcpy(&dev->params, params, sizeof(rail_params_t));


    DEBUG("rail->setup called\n");

    /* default, no promiscuous  mode */
    dev->promiscuousMode = false;

    dev->state = RAIL_TRANSCEIVER_STATE_UNINITIALIZED;
    dev->lastRxPacketHandle = RAIL_RX_PACKET_HANDLE_INVALID;

    /* TODO config for 868/912MHz different? */
    dev->csma_config = _rail_csma_default_config;

}

/* init Packet Trace (PTI) functionality -> usefull for debugging */
#if (RAIL_PTI_ENABLED == 1)
int _rail_PTI_init(rail_t *dev)
{

    /* init gpio for trace output */

    RAIL_PtiConfig_t pti_config = RAIL_PTI_CONFIG;

    RAIL_ConfigPti(dev->rhandle, &pti_config);

    RAIL_EnablePti(dev->rhandle, true);

    DEBUG("RADIO_PTI_Init done\n");

    return 0;
}
#endif

/* initialisation of the transceivers power amplifier
   have to be called in init
   TODO what about deep sleep?
 */
int _rail_PA_init(rail_t *dev)
{

    /* Initialize the RAIL Tx power curves for all PAs on this chip */
    RAIL_TxPowerCurvesConfig_t tx_power_curves_config = {
        curves24Hp,
        curvesSg,
        curves24Lp,
        piecewiseSegments
    };

    RAIL_Status_t ret = RAIL_InitTxPowerCurves(&tx_power_curves_config);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Error init PA  (Tx power curves) for rail - error msg: %s\n", rail_error2str(ret));
        return -1;
    }

    /* Power config, depends on chip etc ... */
    /* TODO
         - multi freq, mult protocol
         - there are SoCs with high and low power PAs,
            how to determain which version has this SoC?
            atm only 2.4GHZ HighPower is supported
     */
    RAIL_TxPowerConfig_t tx_power_config = {
#if RAIL_RADIO_BAND == 2400
        RAIL_TX_POWER_MODE_2P4_HP,
#elif (RAIL_RADIO_BAND == 868) || (RAIL_RADIO_BAND == 915)
        RAIL_TX_POWER_MODE_SUBGIG,
#endif
        RAIL_RADIO_PA_VOLTAGE, /* voltage vPA for the DCDC connection */
        10
    };

    ret = RAIL_ConfigTxPower(dev->rhandle, &tx_power_config);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Error init PA  (config Tx power) for rail - error msg: %s\n", rail_error2str(ret));
        return -1;
    }

    ret = RAIL_SetTxPowerDbm(dev->rhandle, ((RAIL_TxPower_t)RAIL_DEFAULT_TXPOWER) * 10);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Error init PA  (set tx power) for rail");
        return -1;
    }

    DEBUG("RADIO_PA_Init done\n");

    return 0;
}

int rail_init(rail_t *dev)
{

    RAIL_Status_t ret;
    int r = 0;

    netdev_ieee802154_t *netdev = (netdev_ieee802154_t *)dev;

    /* save ref for this driver, for global access (esp in rail event handler) */
    /* TODO multible instances? */
    _rail_dev = dev;

    DEBUG("rail_init called\n");

    dev->state = RAIL_TRANSCEIVER_STATE_UNINITIALIZED;

    /* start with long addr mode. */
    netdev->flags |= NETDEV_IEEE802154_SRC_MODE_LONG;

    /* get informations about the used raillib */
    /* TODO check if driver is compatible? */
    RAIL_Version_t rail_version;
    RAIL_GetVersion(&rail_version, true);

    DEBUG("Using Silicon Labs RAIL Lib. Version %u.%u Rev: %u build: %u multiprotocol: %s \n",
          rail_version.major, rail_version.minor, rail_version.rev, rail_version.build,
          rail_version.multiprotocol ? "YES" : "NO");

    /* init rail blob config
       set to zero, because API manual request it
     */
    memset(&(dev->rconfig), 0, sizeof(RAIL_Config_t));

    dev->rconfig.eventsCallback = &_rail_radio_event_handler;
    dev->rconfig.protocol = NULL;
    dev->rconfig.scheduler = NULL;

    /* Init rail driver blob instance */
    dev->rhandle = RAIL_Init(&(dev->rconfig), NULL);

    if (dev->rhandle == NULL) {
        LOG_ERROR("Can not init rail blob driver\n");
        return -1;
    }

    /* config data management, easier version with packets */

    static const RAIL_DataConfig_t rail_data_config = {
        TX_PACKET_DATA,
        RX_PACKET_DATA,
        PACKET_MODE,
        PACKET_MODE,
    };

    ret = RAIL_ConfigData(dev->rhandle, &rail_data_config);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can not init rail data config - error msg: %s\n", rail_error2str(ret));
        return -1;
    }

    /* config transceiver calibration (freq depends on temperature etc) */

    ret = RAIL_ConfigCal(dev->rhandle, RAIL_CAL_ALL);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can not init rail calibration - error msg: %s\n", rail_error2str(ret));
        return -1;
    }

    /* configure the channels for 802.15.4 */


#if RAIL_RADIO_BAND == 2400
    /* for 2.4 GHz the RAIL API provides a std conform default config */
    DEBUG("using 2.4GHz radio band\n");
    ret = RAIL_IEEE802154_Config2p4GHzRadio(dev->rhandle);

#elif (RAIL_RADIO_BAND == 868) || (RAIL_RADIO_BAND == 915)
    /* there is no default config, so we have to do it manually (or copied from
       openthread/mbed)
       currently the calls are the same, might change if RAIL API functions
       became available
     */
#if (RAIL_RADIO_BAND == 868)
    DEBUG("using 868MHz radio band\n");
    ret = RAIL_ConfigChannels(dev->rhandle, &_rail_radio_channel_config, NULL);
#elif (RAIL_RADIO_BAND == 915)
    DEBUG("using 915MHz radio band\n");
    ret = RAIL_ConfigChannels(dev->rhandle, &_rail_radio_channel_config, NULL);
#endif

#endif /*  (RAIL_RADIO_BAND == 868) || (RAIL_RADIO_BAND == 915) */

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can not init rail radio channels - error msg: %s\n", rail_error2str(ret));
        return -1;
    }

    /* init IEEE802.15.4 acceleration features  */

    ret = RAIL_IEEE802154_Init(dev->rhandle, &_rail_ieee802154_config);
    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can not init rail ieee 802.15.4 support - error msg: %s\n", rail_error2str(ret));
        return -1;
    }

    /* init radio PA */

    r = _rail_PA_init(dev);

    if (r != 0) {
        return r;
    }

    /* setup transmitt buffer */
    ret = RAIL_SetTxFifo(dev->rhandle, _transmit_buffer, 0, sizeof(_transmit_buffer));

    /* setup PTI */
#if (RAIL_PTI_ENABLED == 1)
    r = _rail_PTI_init(dev);
    assert(r == 0);
#endif

    /* TODO
         - how to figure out if this device is the PAN coord?
     */


    /* get mac addr from SoC */
    /* this is a bit messy, because everthing has or what it in different
       endianess
       for convenience we read it once and save it in the netdev structure in
       big endianess
     */
    uint32_t tmp = DEVINFO->UNIQUEL;
    dev->eui.uint64 = byteorder_htonll((uint64_t)((uint64_t)DEVINFO->UNIQUEH << 32) | tmp);

    DEBUG("Node EUI: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          dev->eui.uint8[0],
          dev->eui.uint8[1],
          dev->eui.uint8[2],
          dev->eui.uint8[3],
          dev->eui.uint8[4],
          dev->eui.uint8[5],
          dev->eui.uint8[6],
          dev->eui.uint8[7]);

    /* TODO provide default PANID / NID by params? */
    DEBUG("Set PanID to 0x%04x\n", RAIL_DEFAULT_PANID);

    netdev->pan = RAIL_DEFAULT_PANID;

    bool bRet = RAIL_IEEE802154_SetPanId(dev->rhandle, RAIL_DEFAULT_PANID, 0);
    if (bRet != true) {
        DEBUG("Can not set PAN ID %d\n", RAIL_DEFAULT_PANID);
    }

    /* set short addr */
    DEBUG("Set ShortAddr 0x%04x\n", ntohs(dev->eui.uint16[3].u16));

    /* yeah riot want it in big endian*/
    memcpy(netdev->short_addr, &dev->eui.uint16[3].u16, 2);

    /* rail want it in little endian ...*/
    bRet = RAIL_IEEE802154_SetShortAddress(dev->rhandle, byteorder_ntohs(dev->eui.uint16[3]), 0);
    if (bRet != true) {
        DEBUG("Can not set short addr\n");
    }

    /* set long addr */
    DEBUG("Set LongAddr 0x%08lx%08lx\n", ntohl(dev->eui.uint64.u32[0]), ntohl(dev->eui.uint64.u32[1]));

    memcpy(netdev->long_addr, &dev->eui.uint8, IEEE802154_LONG_ADDRESS_LEN);

    /* and for the long address, it have to be little endian aka reversed order */
    uint64_t addr_rev = byteorder_ntohll(dev->eui.uint64);

    bRet = RAIL_IEEE802154_SetLongAddress(dev->rhandle, (uint8_t *)&addr_rev, 0);
    if (bRet != true) {
        DEBUG("Can not set long addr\n");
    }

    /* get transmitt power, TODO only for debug mode */

    RAIL_TxPower_t power_tx_ddBm = RAIL_GetTxPowerDbm(dev->rhandle);

    DEBUG("TX Power set to %hd deci dBm\n", power_tx_ddBm);

    /* configure the RAIL driver blob events we want the receive
       it is possible to get all events with RAIL_EVENTS_ALL
     */

    ret = RAIL_ConfigEvents(dev->rhandle,
                            RAIL_EVENTS_ALL,    /* mask of events, which should be modified, here all */
                                                /* events to subscribe */
                            RAIL_EVENT_RX_ACK_TIMEOUT |
                            RAIL_EVENT_RX_FRAME_ERROR |
                            RAIL_EVENT_RX_ADDRESS_FILTERED |
                            RAIL_EVENT_RX_PACKET_RECEIVED |
                            RAIL_EVENT_TX_PACKET_SENT |
                            RAIL_EVENT_TX_CHANNEL_BUSY |
                            RAIL_EVENT_TX_ABORTED |
                            RAIL_EVENT_TX_BLOCKED |
                            RAIL_EVENT_TX_UNDERFLOW |
                            RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND |
                            RAIL_EVENT_CAL_NEEDED);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can not subsripe to rail events - error msg: %s", rail_error2str(ret));
        return -1;
    }

    /* TODO use / set some state? */

    DEBUG("rail_init done\n");
    LOG_INFO("rail radio driver initialised\n");

    return 0;
}

/* TODO
    - docu
    - rename to transmit_packet, frames are done a layer above
 */
int rail_transmit_frame(rail_t *dev, uint8_t *data_ptr, size_t data_length)
{
    DEBUG("rail_transmit_frame called\n");

    /* set frame length to first byte (I wonder where this is actually documented?) */
    data_ptr[0] = (uint8_t)data_length + 1;

    /* force radio state to idle, aboard running ops, so we can transmitt
       otherwise the transceiver might be receiving/transmitting and the new 
       transmit op fails.
       TODO ensure there are no other running ops
    */
    RAIL_Idle(dev->rhandle, RAIL_IDLE_ABORT, true);


    /* write packet payload in the buffer of the rail driver blob*/
    RAIL_WriteTxFifo(dev->rhandle, data_ptr, data_length + 1, true);

    /* config tx options, here just the defaults*/
    RAIL_TxOptions_t tx_option = RAIL_TX_OPTIONS_DEFAULT;

    /* TODO internal state */
    dev->state = RAIL_TRANSCEIVER_STATE_TX;

    /* check if ack req is requested */
    if (dev->netdev.flags & NETDEV_IEEE802154_ACK_REQ) {
        tx_option |= RAIL_TX_OPTION_WAIT_FOR_ACK;
        DEBUG("tx option auto ack\n");
        /* TODO wait for ack, necessary or done by layer above? */
    }

    DEBUG("[rail] transmit - radio state: %s\n", rail_radioState2str(RAIL_GetRadioState(dev->rhandle)));

    /* start tx with settings in csma_config
     */
    RAIL_Status_t ret = RAIL_StartCcaCsmaTx(dev->rhandle,
                                            dev->netdev.chan,
                                            tx_option,
                                            &dev->csma_config,
                                            NULL);

    if (ret != RAIL_STATUS_NO_ERROR) {
        LOG_ERROR("Can't start transmit - state %s -  error msg: %s \n", 
                rail_radioState2str(RAIL_GetRadioState(dev->rhandle)),
                rail_error2str(ret));
        return -1;
    }
    DEBUG("Started transmit\n");

    /* TODO
       - if this should be asymmetric blocking call, we have to wait for the
         tx done event by the callback
        - or use while (RAIL_GetRadioState(dev->rhandle) & RAIL_RF_STATE_TX );
     */
    return 0;
}

int rail_start_rx(rail_t *dev)
{

    /*
       TODO process:
        check state
        if uninit/init etc/ -> error;
        if calibrate || error
            -> tx error
        if idle and rx
            if waiting for ack
                no timeout? error
            timeout?, stats++, cont
     */
    /* check if set? or just a global setting? */
    if (dev->promiscuousMode == true) {
        RAIL_IEEE802154_SetPromiscuousMode(dev->rhandle, true);
    }
    else {
        RAIL_IEEE802154_SetPromiscuousMode(dev->rhandle, false);
    }


    /* set channel to listen to */
    RAIL_StartRx(dev->rhandle, dev->netdev.chan, NULL);
    dev->state = RAIL_TRANSCEIVER_STATE_RX;
    return 0;
}

/* RAIL blob event handler, this is not a ISR handler! */
/* TODO what parts can be moved to the netdev->_isr function?
 */
static void _rail_radio_event_handler(RAIL_Handle_t rhandle, RAIL_Events_t event)
{

    /* TODO get the right netdev struct */
    rail_t *dev = _rail_dev;

    /* rail events are a bitmask, therefore multible events within this call
       possible
     */

    /* event description c&p from RAIL API docu */

    /* Notifies the application when searching for an ack packet has timed out */
    if (event & RAIL_EVENT_RX_ACK_TIMEOUT) {
        DEBUG("Rail event RX ACK TIMEOUT\n");
        /* ack timeout for tx acks? TODO confirm */
        dev->netdev.netdev.event_callback((netdev_t *)&dev->netdev, NETDEV_EVENT_TX_NOACK);
    }

    /* Occurs when a packet being received has a frame error */
    if (event & RAIL_EVENT_RX_FRAME_ERROR) {
        DEBUG("Rail event RX frame error\n");
        dev->netdev.netdev.event_callback((netdev_t *)&dev->netdev, NETDEV_EVENT_CRC_ERROR);

        /* TODO statistic? */
    }

    /* Occurs when a packet's address does not match the filtering settings */
    if (event & RAIL_EVENT_RX_ADDRESS_FILTERED) {
        DEBUG("Rail event rx address filtered\n");
    }

    /*	Occurs whenever a packet is received */
    if (event & RAIL_EVENT_RX_PACKET_RECEIVED) {
        DEBUG("Rail event rx packet received\n");

        /* check if packet is ok */
        RAIL_RxPacketInfo_t rx_packet_info;
        RAIL_RxPacketHandle_t rx_handle;
        rx_handle = RAIL_GetRxPacketInfo(rhandle,
                                         RAIL_RX_PACKET_HANDLE_NEWEST,
                                         &rx_packet_info);

        DEBUG("[rail] rx packet event - len p 0x%02x - len2 0x%02x\n",
              rx_packet_info.firstPortionData[0], rx_packet_info.packetBytes);

        if (rx_packet_info.packetStatus != RAIL_RX_PACKET_READY_SUCCESS) {
            /* error */

            DEBUG("Got an packet with an error - packet status msg: %s \n",
                  rail_packetStatus2str(rx_packet_info.packetStatus));

            dev->netdev.netdev.event_callback((netdev_t *)&dev->netdev, NETDEV_EVENT_CRC_ERROR);
            RAIL_ReleaseRxPacket(rhandle, rx_handle);
        }
        else {
            DEBUG("Rail event rx packet good packet \n");
            /* hold packet so it can be received from netdev thread context */
            RAIL_HoldRxPacket(rhandle);

            /* delegate to the _isr, even if there is nothing to do ...*/
            dev->netdev.netdev.event_callback((netdev_t *)&dev->netdev, NETDEV_EVENT_ISR);
        }
    }

    /* TODO RAIL_EVENT_RX_PACKET_ABORTED */
    /* Occurs when a packet is aborted, but a more specific reason (such as
       RAIL_EVENT_RX_ADDRESS_FILTERED) isn't known.
     */

    /* Occurs when a packet was sent */
    if (event & RAIL_EVENT_TX_PACKET_SENT) {
        DEBUG("Rail event Tx packet sent \n");

        dev->netdev.netdev.event_callback((netdev_t *)&dev->netdev, NETDEV_EVENT_TX_COMPLETE);

        /* TODO set state? */
    }


    /* TODO RAIL_EVENT_TXACK_PACKET_SENT */
    /* Occurs when an ack packet was sent. */

    if (event & RAIL_EVENT_TX_CHANNEL_BUSY) {
        DEBUG("Rail event Tx channel busy\n");
        dev->netdev.netdev.event_callback((netdev_t *)&dev->netdev, NETDEV_EVENT_TX_MEDIUM_BUSY);

        /* TODO set state? */
    }

    /* Occurs when a transmit is aborted by the user */
    if (event & RAIL_EVENT_TX_ABORTED) {
        DEBUG("Rail event Tx aborted\n");

        /* TODO set state? */
    }

    /* TODO RAIL_EVENT_TXACK_ABORTED */
    /* Occurs when a transmit is aborted by the user */

    /*  Occurs when a transmit is blocked from occurring due to having called
        RAIL_EnableTxHoldOff().
     */
    if (event & RAIL_EVENT_TX_BLOCKED) {
        DEBUG("Rail event Tx blocked\n");

        /* TODO how to notify layer above? */
        /* TODO set state? */
    }

    /* TODO RAIL_EVENT_TXACK_BLOCKED */
    /*  Occurs when an ack transmit is blocked from occurring due to having
        called RAIL_EnableTxHoldOff().
     */

    /* Occurs when the transmit buffer underflows. */
    if (event & RAIL_EVENT_TX_UNDERFLOW) {
        LOG_INFO("Rail event Tx underflow - > should not happen: race condition" 
                " while transmitting new package\n");
        /* should not happen as long as the packet is written as whole into the
           RAIL driver blob buffer*/
    }

    /* RAIL_EVENT_TXACK_UNDERFLOW */
    /* Occurs when the ack transmit buffer underflows.*/

    /* Indicates a Data Request is being received when using IEEE 802.15.4
       functionality. */
    if (event & RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND) {
        /* TODO what is source match? and why might it be necessary to filter
           here the packet?
         */
        DEBUG("Rail event ieee 802.15.4 data request command\n");
        RAIL_IEEE802154_SetFramePending(rhandle);
    }

    /* Occurs when the application needs to run a calibration.*/
    if (event & RAIL_EVENT_CAL_NEEDED) {

        DEBUG("Rail event calibration needed \n");
        LOG_INFO("Rail radio transceiver needs a calibration: executed\n");

        RAIL_Status_t ret;

        ret = RAIL_Calibrate(rhandle, NULL, RAIL_CAL_ALL_PENDING);

        DEBUG("calibration done, ret: %d \n", ret);

        assert(ret == RAIL_STATUS_NO_ERROR);
    }
}

#ifdef DEVELHELP
const char *rail_error2str(RAIL_Status_t status)
{

    switch (status) {
        case (RAIL_STATUS_NO_ERROR):
            return "No error";
        case (RAIL_STATUS_INVALID_PARAMETER):
            return "Invalid parameter";
        case (RAIL_STATUS_INVALID_STATE):
            return "Invalid state";
        case (RAIL_STATUS_INVALID_CALL):
            return "Invalid Call";
        case (RAIL_STATUS_SUSPENDED):
            return "Status suspended";
    }
    return "Error code unknown";
}

const char *rail_packetStatus2str(RAIL_RxPacketStatus_t status)
{

    switch (status) {
        case (RAIL_RX_PACKET_NONE):
            return "Radio is idle or searching for a packet.";
        case (RAIL_RX_PACKET_ABORT_FORMAT):
            return "Format/Length error.";
        case (RAIL_RX_PACKET_ABORT_FILTERED):
            return "Filtering error (address).";
        case (RAIL_RX_PACKET_ABORT_ABORTED):
            return "Aborted error.";
        case (RAIL_RX_PACKET_ABORT_OVERFLOW):
            return "Receive overflowed buffer.";
        case (RAIL_RX_PACKET_ABORT_CRC_ERROR):
            return "CRC error aborted.";
        case (RAIL_RX_PACKET_READY_CRC_ERROR):
            return "CRC error accepted (details available).";
        case (RAIL_RX_PACKET_READY_SUCCESS):
            return "Success (details available).";
        case (RAIL_RX_PACKET_RECEIVING):
            return "Receiving in progress.";
        default:
            return "Unknown status";
    }
}

const char *rail_radioState2str(RAIL_RadioState_t state)
{
    switch (state) {
        case (RAIL_RF_STATE_INACTIVE):
            return "state inactive";
        case (RAIL_RF_STATE_ACTIVE):
            return "state active / idle";
        case (RAIL_RF_STATE_RX):
            return "state rx";
        case (RAIL_RF_STATE_TX):
            return "state tx";
        default:
            return "unknown state";
    }
}
#endif /* DEVELHELP */
