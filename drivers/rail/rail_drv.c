
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "radio.h"

// gecko sdk rail lib includes
#include "em_core.h"
#include "rail.h"
#include "pti.h"
#include "pa.h"
#include "rail_assert_error_codes.h"
#include "ieee802154/rail_ieee802154.h"

// riot os rail driver includes
#include "rail_drv.h"
#include "rail_registers.h"
#include "rail_internal.h"
#include "rail_netdev.h"

#include "net/ieee802154.h"


#define ENABLE_DEBUG (1)
#include "debug.h"

#include "log.h"

// receive buffer
//static uint8_t _receiveBuffer[IEEE802154_FRAME_LEN_MAX + 1 + sizeof(RAIL_RxPacketInfo_t)];
static bool _receiveBufferIsAllocated = false;

static volatile bool _rfReady = false;

void rail_setup(rail_t* dev, const rail_params_t* params)
{
    
    netdev2_t *netdev = (netdev2_t *)dev;
    
    // register driver (defined in rail_netdev)
    netdev->driver = &rail_driver;
    
    // init dev 
    memcpy(&dev->params, params, sizeof(rail_params_t));
    
    // ?? todo
    
    DEBUG("rail->setup called\n");
    
    dev->state = RAIL_TRANSCEIVER_STATE_UNINITIALISIED;
    
    // init other hal stuff
    rail_internal_init_radio_hal(params);
    
    //// init radio
  //  RAIL_RfIdle();
  //  dev->state = RAIL_TRANSCEIVER_STATE_IDLE;
    
    
}

int initPTI(rail_t* dev) {

    // init gpio for output

    RADIO_PTIInit_t ptiInit = RADIO_PTI_INIT;
    RADIO_PTI_Init(&ptiInit);
    DEBUG("RADIO_PTI_Init done\n");

    return 0;
}

int rail_init(rail_t* dev)
{
    DEBUG("rail_init called\n");
    // c&p from openthread
    
    // PTI init
#if (PTI_ENABLED)
    initPTI(dev);
#endif

    // PA init
    RADIO_PAInit_t paInit = (RADIO_PAInit_t) RADIO_PA_2P4_INIT;
    if (!RADIO_PA_Init(&paInit)) {
        // Error: The PA could not be initialized due to an improper 
        // configuration.
        // Please ensure your configuration is valid for the selected part.
        LOG_ERROR("Can't init rail radio PM\n");
        assert(false);
    }
    DEBUG("RADIO_PA_Init done\n");
    
    // radio debug?
    // TODO radio debug
     

    // RfInit
    RAIL_Init_t railInitParams =
    {
        128, // maxPacketLength: UNUSED
        CLOCK_HFXO_FREQ,   // frequency of the external crystal
        0    // calEnable  mask defines cal to perfom in RAIL
    };
    
    uint8_t ret ;
    ret = RAIL_RfInit(&railInitParams);
    
    if (ret != 0) {
        LOG_ERROR("Can not init RAIL radio: error code: %u\n", ret);
        return -1;
    }
    // wait till rf is ready
    while (_rfReady == false);

    // CalInit calibrate the radio transceiver
    // TODO

    RAIL_Status_t r;
    // 802.15.4 RadioConfig
    // if 2.4 GHz
    r = RAIL_IEEE802154_2p4GHzRadioConfig();
    if (r != RAIL_STATUS_NO_ERROR){
        assert(false);
    }
    // 802 init
    RAIL_IEEE802154_Config_t config = { false,  // promiscuousMode
                                        false,  // isPanCoordinator
                                        RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES, // framesMask, which frame will be received
                                        RAIL_RF_STATE_RX, // defaultState, state after transmitt
                                        100,   // idleTime time to go from idle to RX or TX
                                        192,   // turnaroundTime time after receiving a packet and transmitting an ack
                                        894,   // ackTimeout
                                        NULL   // addresses, address filter, to allow only the given addresses
                                       };
    r = RAIL_IEEE802154_Init(&config);
    if (r != RAIL_STATUS_NO_ERROR) {
        assert(false);
    }
    // if pan coord
    // setpancoord
    // get mac addr
    memcpy(dev->mac_address, (const void*)&DEVINFO->UNIQUEH, 4);
    memcpy(dev->mac_address+4, (const void*)&DEVINFO->UNIQUEL, 4);
    // set panid
    // set short addr
    // set long addr
    // txpowerset


    // Data Management allready the default
    RAIL_DataConfig_t railDataConfig =
    {
        TX_PACKET_DATA,
        RX_PACKET_DATA,
        PACKET_MODE,
        PACKET_MODE,
    };

    
    r = RAIL_DataConfig(&railDataConfig);
    if (r != RAIL_STATUS_NO_ERROR) {
        assert(false);
    }
    
        // 802.15.4 configuration
    
    RAIL_TxPowerSet(dev->params.max_transit_power);
    
    RAIL_RfIdle();
    dev->state = RAIL_TRANSCEIVER_STATE_IDLE;
    
    
    DEBUG("rail_init done\n");
    LOG_INFO("rail radio initialised\n");
    
    return 0;
}

int rail_tx_prepare(rail_t* dev)
{
    
    
    return 0;
}



// start impl for the RAIL callback functions needed by the rail radio lib
// docu c&p from rail-library-callbacks.info

/**
 * Callback that notifies the application when searching for an ACK has timed
 * out.
 *
 * @return void
 *
 * This callback function is called whenever the timeout for searching for an
 * ack
 */
void RAILCb_RxAckTimeout(void)
{
   // sTransmitError = OT_ERROR_NO_ACK;
      DEBUG("RailCB RxAckTimeout \n");
}

/**
 * Callback that lets the app know when the radio has finished init
 * and is ready.
 */
void RAILCb_RfReady(void)
{
    DEBUG("RailCB RFReady \n");
    _rfReady = true;
}

/**
 * Interrupt level callback to signify when the packet was sent
 * @param txPacketInfo Information about the packet that was transmitted.
 * @note that this structure is only valid during the timeframe of the
 * callback.
 */
void RAILCb_TxPacketSent(RAIL_TxPacketInfo_t *aTxPacketInfo)
{
    (void)aTxPacketInfo;
     DEBUG("RailCB TxPacketSent \n");
   // sTransmitError = OT_ERROR_NONE;
   // sTransmitBusy = false;
}


/**
 * Interrupt level callback
 * Allows the user finer granularity in tx radio events.
 *
 * Radio Statuses:
 * RAIL_TX_CONFIG_BUFFER_UNDERFLOW
 * RAIL_TX_CONFIG_CHANNEL_BUSY
 *
 * @param[in] status A bit field that defines what event caused the callback
 */
void RAILCb_TxRadioStatus(uint8_t status) 
{
    DEBUG("RailCB TxRadioStatus status 0x%2x \n", status);
}

/**
 * Callback that notifies the application that a calibration is needed.
 *
 * This callback function is called whenever the RAIL library detects that a
 * calibration is needed. It is up to the application to determine a valid
 * window to call RAIL_CalStart().
 *
 */
void RAILCb_CalNeeded(void)
{
    DEBUG("RailCB CalNeeded \n");
}


/**
 * Called whenever an enabled radio status event occurs
 *
 * Triggers:
 *  RAIL_RX_CONFIG_PREAMBLE_DETECT
 *  RAIL_RX_CONFIG_SYNC1_DETECT
 *  RAIL_RX_CONFIG_SYNC2_DETECT
 *  RAIL_RX_CONFIG_INVALID_CRC
 *  RAIL_RX_CONFIG_BUFFER_OVERFLOW
 *  RAIL_RX_CONFIG_ADDRESS_FILTERED
 *
 * @param[in] status The event that triggered this callback
 */
void RAILCb_RxRadioStatus(uint8_t aStatus)
{
    (void)aStatus;
    DEBUG("RailCB RxRadioStatus status 0x%2x \n", aStatus);
}


/**
 * Receive packet callback.
 *
 * @param[in] rxPacketHandle Contains a handle that points to the memory that
 *   the packet was stored in. This handle will be the same as something
 *   returned by the RAILCb_AllocateMemory() API. To convert this into a receive
 *   packet info struct use the *** function.
 *
 * This function is called whenever a packet is received and returns to you the
 * memory handle for where this received packet and its appended information was
 * stored. After this callback is done we will release the memory handle so you
 * must somehow increment a reference count or copy the data out within this
 * function.
 */
void RAILCb_RxPacketReceived(void *rxPacketHandle) {
    DEBUG("RailCB RxPacketReceived  \n");
}


/**
 * Interrupt level callback to signify when the radio changes state.
 *
 * @param[in] state Current state of the radio, as defined by EFR32 data sheet
 * TODO: Unify these states with the RAIL_RadioState_t type? (There are much
 *    more than just TX, RX, and IDLE)
 */
void RAILCb_RadioStateChanged(uint8_t aState)
{
    (void)aState;
    DEBUG("RailCB RadioStateChanged status 0x%2x \n", aState);
}



/**
 * Callback function used by RAIL to request memory.
 *
 * @param[in] size The amount of memory in bytes that we need for this packet
 * @return A handle to memory in your storage system.
 *
 * This is used to allocate memory for receive packets and must be implemented
 * by the application.
 */
void *RAILCb_AllocateMemory(uint32_t aSize)
{
    
    DEBUG("RailCB AllocateMemory size: %"PRIu32" \n", aSize);
    
    // todo howto make sure there is only one buffer alloced?
    //uint8_t* pointer = NULL;
    // todo replace with riot irq functions
    //CORE_DECLARE_IRQ_STATE;
    /*
    if(aSize > (IEEE802154_FRAME_LEN_MAX + 1 + sizeof(RAIL_RxPacketInfo_t))) {
        LOG_ERROR("Received package is to big for buffer (size %"PRIu32")\n", aSize);
        return NULL;
    }

    CORE_ENTER_CRITICAL();
    if (_receiveBufferIsAllocated == false) {
        _receiveBufferIsAllocated = true;
        pointer = _receiveBuffer;
    }

*/
//exit:
 //  CORE_EXIT_CRITICAL();
//    return pointer;
    return NULL;
}

/**
 * Called to begin copying received data into the current memory handle.
 *
 * @param[in] handle A handle to the current memory block for packet data.
 * @param[in] offset The offset in bytes from the start of the handle that we
 * need a pointer for.
 * @param[out] available The number of bytes available to be written to this
 * return pointer. If this is zero the receive will terminate. This parameter
 * will default to all spaces allocated to handle contiguous allocators. If your
 * allocator is different you *must* set this appropriately.
 * @return A pointer to the address to write data for this handle.
 *
 * This function is called before every memory write to a handle so that we can
 * get the actual address this handle references in the system. When we're done
 * writing there will be a corresponding call to RAILCb_EndWriteMemory().
 *
 * @note You must have at least sizeof(RAIL_RxPacketInfo_t) contiguous bytes at
 * offset 0 or the appended info will not be written.
 */
void *RAILCb_BeginWriteMemory(void *aHandle, uint32_t aOffset,
                              uint32_t *available)
{
    (void)available;
    DEBUG("RailCB BeginWriteMemory\n");
    return ((uint8_t *)aHandle) + aOffset;
}

/**
 * Called to complete the write memory transaction.
 *
 * @param handle The handle to the current memory block we're modifying.
 * @param offset The offset in bytes from the start of the handle that this data
 * was written to.
 * @param size The number of bytes that were written.
 *
 * This callback indicates the completeion of a write memory transaction. It
 * can be used to store information about how many bytes were written or 
 * anything else needed. Once this is called the pointer returned by 
 * RAILCb_BeginWriteMemory() will no longer be assumed to be valid and we will
 * call that function again for any future writes. 
 */

void RAILCb_EndWriteMemory(void *aHandle, uint32_t aOffset, uint32_t aSize)
{
    (void)aHandle;
    (void)aOffset;
    (void)aSize;
    DEBUG("RailCB EndWriteMemory\n");
}

/**
 * Callback function used by RAIL to free memory.
 *
 * @param[in] handle A handle to a memory block allocated with the
 *   RAILCb_AllocateMemory() API above.
 *
 * This is used to free memory that was allocated with the
 * RAILCb_AllocateMemory() function when RAIL is done using it.
 */

void RAILCb_FreeMemory(void *aHandle)
{
    (void)aHandle;
    DEBUG("RailCB FreeMemory\n");
    
    CORE_CRITICAL_SECTION(
        _receiveBufferIsAllocated = false;
    );
}

/**
 * This function is called when the RAIL timer expires
 *
 * You must implement a stub for this in your RAIL application even if you
 * don't use the timer.
 */
void RAILCb_TimerExpired(void)
{
    DEBUG("RailCB TimerExpired\n");
}



/**
 * Callback that fires when the transmit fifo falls under the configured
 * threshold value
 *
 * @param[in] spaceAvailable Number of bytes open in the transmit fifo at the
 * time of the callback dispatch
 *
 * @return void
 * @warning You must implement a stub for this in your RAIL application.
 *
 * Callback that fires when the transmit fifo falls under the configured
 * threshold value. It only fires if a rising edge occurs across this
 * threshold. This callback will not fire on initailization nor after resetting
 * the transmit fifo with RAIL_ResetFifo().
 *
 * Provides the number of bytes open in the transmit fifo at the time of the
 * callback dispatch.
 */
void RAILCb_TxFifoAlmostEmpty(uint16_t spaceAvailable)
{
    DEBUG("RailCB TxFifoAlmostEmpty\n");
}

/**
 * Callback for when AGC averaged RSSI is done
 *
 * @param avgRssi Contains the the RSSI in quarter dBm (dbm*4) on success and
 * returns \ref RAIL_RSSI_INVALID if there was a problem computing the result.
 *
 * Called in response to RAIL_StartAverageRSSI() to indicate that the hardware
 * has completed averaging. If you would like you can instead use the
 * RAIL_AverageRSSIReady() to wait for completion and RAIL_GetAverageRSSI() to
 * get the result.
 */
void RAILCb_RssiAverageDone(int16_t avgRssi)
{
    DEBUG("RailCB RssiAverageDone avgRssi %"PRIi16"\n", avgRssi);
}

/**
 * Callback that fires when the receive fifo exceeds the configured threshold
 * value
 *
 * @param[in] bytesAvailable Number of bytes available in the receive fifo at
 * the time of the callback dispatch
 *
 * @return void
 * @warning You must implement a stub for this in your RAIL application.
 *
 * Callback that fires when the receive fifo exceeds the configured threshold
 * value.  Provides the number of bytes available in the receive fifo at the
 * time of the callback dispatch.
 */
void RAILCb_RxFifoAlmostFull(uint16_t bytesAvailable)
{
    DEBUG("RailCB RxFifoAlmostFull bytesAvailable %"PRIi16"\n", bytesAvailable);
}

/**
 * Callback for when a Data Request is being received
 *
 * @param address The source address of the data request command
 *
 * This function is called when the command byte of an incoming frame is for a
 * data request, which requests an ACK. This callback will be called before the
 * packet is fully received, to allow the node to have more time to decide
 * whether to set frame pending in the outgoing ACK.
 */
void RAILCb_IEEE802154_DataRequestCommand(RAIL_IEEE802154_Address_t *address)
{
    DEBUG("RailCB DataRequestCommand \n");
}

void RAILCb_AssertFailed(uint32_t errorCode)
{

  static const char* railErrorMessages[] = RAIL_ASSERT_ERROR_MESSAGES;
  const char *errorMessage = "Unknown";

  // If this error code is within the range of known error messages then use
  // the appropriate error message.
  if (errorCode < (sizeof(railErrorMessages) / sizeof(char*))) {
    errorMessage = railErrorMessages[errorCode];
  }
  // Print a message about the assert that triggered
  LOG_ERROR("rail assert code:%"PRIu32",message:%s",
                errorCode,
                errorMessage);
  // Reset the chip since an assert is a fatal error
  NVIC_SystemReset();
}
