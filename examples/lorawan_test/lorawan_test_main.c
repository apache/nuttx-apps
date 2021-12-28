//  Demo Program for LoRaWAN on NuttX based on:
//  https://github.com/lupyuen/LoRaMac-node-nuttx/blob/master/src/apps/LoRaMac/fuota-test-01/B-L072Z-LRWAN1/main.c
//  https://github.com/lupyuen/LoRaMac-node-nuttx/blob/master/src/apps/LoRaMac/periodic-uplink-lpp/B-L072Z-LRWAN1/main.c
/*!
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2018 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 */
#include <nuttx/config.h>
#include <stdio.h>
#include "firmwareVersion.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/githubVersion.h"
#include "../../../nuttx/libs/liblorawan/src/boards/utilities.h"
#include "../../../nuttx/libs/liblorawan/src/mac/region/RegionCommon.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/Commissioning.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/LmHandler/LmHandler.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/LmHandler/packages/LmhpCompliance.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/LmHandler/packages/LmhpClockSync.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/LmHandler/packages/LmhpRemoteMcastSetup.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/LmHandler/packages/LmhpFragmentation.h"
#include "../../../nuttx/libs/liblorawan/src/apps/LoRaMac/common/LmHandlerMsgDisplay.h"

#ifndef ACTIVE_REGION

#warning "No active region defined, LORAMAC_REGION_AS923 will be used as default."

#define ACTIVE_REGION LORAMAC_REGION_AS923

#endif

/*!
 * LoRaWAN default end-device class
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A

/*!
 * Defines the application data transmission duty cycle. 40s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            40000

/*!
 * Defines a random delay for application data transmission duty cycle. 5s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        5000

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_STATE                           LORAMAC_HANDLER_ADR_OFF

/*!
 * Default datarate
 *
 * \remark Please note that LORAWAN_DEFAULT_DATARATE is used only when ADR is disabled 
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_3

/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_DEFAULT_CONFIRMED_MSG_STATE         LORAMAC_HANDLER_UNCONFIRMED_MSG

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE            242

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        true

/*!
 *
 */
typedef enum
{
    LORAMAC_HANDLER_TX_ON_TIMER,
    LORAMAC_HANDLER_TX_ON_EVENT,
}LmHandlerTxEvents_t;

/*!
 * User application data
 */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t TxTimer;

static void OnMacProcessNotify( void );
static void OnNvmDataChange( LmHandlerNvmContextStates_t state, uint16_t size );
static void OnNetworkParametersChange( CommissioningParams_t* params );
static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn );
static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn );
static void OnJoinRequest( LmHandlerJoinParams_t* params );
static void OnTxData( LmHandlerTxParams_t* params );
static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params );
static void OnClassChange( DeviceClass_t deviceClass );
static void OnBeaconStatusChange( LoRaMacHandlerBeaconParams_t* params );
#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
static void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection );
#else
static void OnSysTimeUpdate( void );
#endif
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static int8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size );
static int8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size );
#endif
static void OnFragProgress( uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost );
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static void OnFragDone( int32_t status, uint32_t size );
#else
static void OnFragDone( int32_t status, uint8_t *file, uint32_t size );
#endif
static void StartTxProcess( LmHandlerTxEvents_t txEvent );
static void UplinkProcess( void );

static void OnTxPeriodicityChanged( uint32_t periodicity );
static void OnTxFrameCtrlChanged( LmHandlerMsgTypes_t isTxConfirmed );
static void OnPingSlotPeriodicityChanged( uint8_t pingSlotPeriodicity );

/*!
 * Function executed on TxTimer event
 */
static void OnTxTimerEvent( struct ble_npl_event *event );

static void init_event_queue(void);
static void handle_event_queue(void *arg);

uint8_t BoardGetBatteryLevel( void ) { return 0; } //// TODO
uint32_t BoardGetRandomSeed( void ) { return 22; } //// TODO

static LmHandlerCallbacks_t LmHandlerCallbacks =
{
    .GetBatteryLevel = BoardGetBatteryLevel,
    .GetTemperature = NULL,
    .GetRandomSeed = BoardGetRandomSeed,
    .OnMacProcess = OnMacProcessNotify,
    .OnNvmDataChange = OnNvmDataChange,
    .OnNetworkParametersChange = OnNetworkParametersChange,
    .OnMacMcpsRequest = OnMacMcpsRequest,
    .OnMacMlmeRequest = OnMacMlmeRequest,
    .OnJoinRequest = OnJoinRequest,
    .OnTxData = OnTxData,
    .OnRxData = OnRxData,
    .OnClassChange= OnClassChange,
    .OnBeaconStatusChange = OnBeaconStatusChange,
    .OnSysTimeUpdate = OnSysTimeUpdate,
};

static LmHandlerParams_t LmHandlerParams =
{
    .Region = ACTIVE_REGION,
    .AdrEnable = LORAWAN_ADR_STATE,
    .IsTxConfirmed = LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
    .TxDatarate = LORAWAN_DEFAULT_DATARATE,
    .PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .DataBufferMaxSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE,
    .DataBuffer = AppDataBuffer,
    .PingSlotPeriodicity = REGION_COMMON_DEFAULT_PING_SLOT_PERIODICITY,
};

static LmhpComplianceParams_t LmhpComplianceParams =
{
    .FwVersion.Value = FIRMWARE_VERSION,
    .OnTxPeriodicityChanged = OnTxPeriodicityChanged,
    .OnTxFrameCtrlChanged = OnTxFrameCtrlChanged,
    .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
};

/*!
 * Defines the maximum size for the buffer receiving the fragmentation result.
 *
 * \remark By default FragDecoder.h defines:
 *         \ref FRAG_MAX_NB   21
 *         \ref FRAG_MAX_SIZE 50
 *
 *         FileSize = FRAG_MAX_NB * FRAG_MAX_SIZE
 *
 *         If bigger file size is to be received or is fragmented differently
 *         one must update those parameters.
 */
#define UNFRAGMENTED_DATA_SIZE                     ( 21 * 50 )

/*
 * Un-fragmented data storage.
 */
static uint8_t UnfragmentedData[UNFRAGMENTED_DATA_SIZE];

static LmhpFragmentationParams_t FragmentationParams =
{
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
    .DecoderCallbacks = 
    {
        .FragDecoderWrite = FragDecoderWrite,
        .FragDecoderRead = FragDecoderRead,
    },
#else
    .Buffer = UnfragmentedData,
    .BufferSize = UNFRAGMENTED_DATA_SIZE,
#endif
    .OnProgress = OnFragProgress,
    .OnDone = OnFragDone
};

/*!
 * Indicates if LoRaMacProcess call is pending.
 * 
 * \warning If variable is equal to 0 then the MCU can be set in low power mode
 */
static volatile uint8_t IsMacProcessPending = 0;

static volatile uint8_t IsTxFramePending = 0;

static volatile uint32_t TxPeriodicity = 0;

/*
 * Indicates if the system time has been synchronized
 */
static volatile bool IsClockSynched = false;

/*
 * MC Session Started
 */
static volatile bool IsMcSessionStarted = false;

/*
 * Indicates if the file transfer is done
 */
static volatile bool IsFileTransferDone = false;

/*
 *  Received file computed CRC32
 */
static volatile uint32_t FileRxCrc = 0;

/*!
 * Main application entry point.
 */

int main(int argc, FAR char *argv[]) {
    //  TODO: BoardInitMcu( );
    //  TODO: BoardInitPeriph( );

    //  Init the LoRaWAN Event Queue
    init_event_queue();

    //  Compute the interval between transmissions based on Duty Cycle
    TxPeriodicity = APP_TX_DUTYCYCLE + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );

    const Version_t appVersion    = { .Value = FIRMWARE_VERSION };
    const Version_t gitHubVersion = { .Value = GITHUB_VERSION };
    DisplayAppInfo( "lorawan_test", 
                    &appVersion,
                    &gitHubVersion );

    //  Init LoRaWAN
    if ( LmHandlerInit( &LmHandlerCallbacks, &LmHandlerParams ) != LORAMAC_HANDLER_SUCCESS )
    {
        printf( "LoRaMac wasn't properly initialized\n" );
        //  Fatal error, endless loop.
        while ( 1 ) {}
    }

    // Set system maximum tolerated rx error in milliseconds
    LmHandlerSetSystemMaxRxError( 20 );

    // The LoRa-Alliance Compliance protocol package should always be initialized and activated.
    LmHandlerPackageRegister( PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams );
    LmHandlerPackageRegister( PACKAGE_ID_CLOCK_SYNC, NULL );
    LmHandlerPackageRegister( PACKAGE_ID_REMOTE_MCAST_SETUP, NULL );
    LmHandlerPackageRegister( PACKAGE_ID_FRAGMENTATION, &FragmentationParams );

    IsClockSynched     = false;
    IsFileTransferDone = false;

    //  Join the LoRaWAN Network
    LmHandlerJoin( );

    //  Set the Transmit Timer
    StartTxProcess( LORAMAC_HANDLER_TX_ON_TIMER );

    //  Handle LoRaWAN Events
    handle_event_queue(NULL);  //  Never returns

    return 0;
}

/*!
 * Prepares the payload of the frame and transmits it.
 */
static void PrepareTxFrame( void )
{
    if (LmHandlerIsBusy()) { puts("PrepareTxFrame: Busy"); return; }

    //  Send a message to LoRaWAN
    const char msg[] = "Hi NuttX";
    printf("PrepareTxFrame: Transmit to LoRaWAN: %s (%d bytes)\n", msg, sizeof(msg));

    //  Compose the transmit request
    memcpy(AppDataBuffer, msg, sizeof(msg));
    LmHandlerAppData_t appData =
    {
        .Buffer = AppDataBuffer,
        .BufferSize = sizeof(msg),
        .Port = 1,
    };

    //  Check if the message can be transmitted
    LoRaMacTxInfo_t txInfo;
    LoRaMacStatus_t status = LoRaMacQueryTxPossible(appData.BufferSize, &txInfo);
    printf("PrepareTxFrame: status=%d, maxSize=%d, currentSize=%d\n", status, txInfo.MaxPossibleApplicationDataSize, txInfo.CurrentPossiblePayloadSize);
    assert(status == LORAMAC_STATUS_OK);

    //  Transmit the message
    if( LmHandlerSend( &appData, LmHandlerParams.IsTxConfirmed ) == LORAMAC_HANDLER_SUCCESS )
    {
        puts("PrepareTxFrame: Transmit OK");
    }
}

static void StartTxProcess( LmHandlerTxEvents_t txEvent )
{
    puts("StartTxProcess");
    switch( txEvent )
    {
    default:
        // Intentional fall through
    case LORAMAC_HANDLER_TX_ON_TIMER:
        {
            // Schedule 1st packet transmission
            TimerInit( &TxTimer, OnTxTimerEvent );
            TimerSetValue( &TxTimer, TxPeriodicity );
            OnTxTimerEvent( NULL );
        }
        break;
    case LORAMAC_HANDLER_TX_ON_EVENT:
        {
        }
        break;
    }
}

static void UplinkProcess( void )
{
    puts("UplinkProcess");
    uint8_t isPending = 0;
    CRITICAL_SECTION_BEGIN( );
    isPending = IsTxFramePending;
    IsTxFramePending = 0;
    CRITICAL_SECTION_END( );
    if( isPending == 1 )
    {
        PrepareTxFrame( );
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Event Handlers

/*!
 * Function executed on TxTimer event
 */
static void OnTxTimerEvent( struct ble_npl_event *event )
{
    printf("OnTxTimerEvent: timeout in %ld ms, event=%p\n", TxPeriodicity, event);
    TimerStop( &TxTimer );

    IsTxFramePending = 1;

    // Schedule next transmission
    TimerSetValue( &TxTimer, TxPeriodicity );
    TimerStart( &TxTimer );
}

static void OnMacProcessNotify( void )
{
    IsMacProcessPending = 1;
}

static void OnNvmDataChange( LmHandlerNvmContextStates_t state, uint16_t size )
{
    DisplayNvmDataChange( state, size );
}

static void OnNetworkParametersChange( CommissioningParams_t* params )
{
    DisplayNetworkParametersUpdate( params );
}

static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn )
{
    DisplayMacMcpsRequestUpdate( status, mcpsReq, nextTxIn );
}

static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn )
{
    DisplayMacMlmeRequestUpdate( status, mlmeReq, nextTxIn );
}

static void OnJoinRequest( LmHandlerJoinParams_t* params )
{
    puts("OnJoinRequest");
    DisplayJoinRequestUpdate( params );
    if( params->Status == LORAMAC_HANDLER_ERROR )
    {
        LmHandlerJoin( );
    }
    else
    {
        LmHandlerRequestClass( LORAWAN_DEFAULT_CLASS );
    }
}

static void OnTxData( LmHandlerTxParams_t* params )
{
    puts("OnTxData");
    DisplayTxUpdate( params );
}

static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params )
{
    puts("OnRxData");
    DisplayRxUpdate( appData, params );
}

static void OnClassChange( DeviceClass_t deviceClass )
{
    puts("OnClassChange");
    DisplayClassUpdate( deviceClass );

    switch( deviceClass )
    {
        default:
        case CLASS_A:
        {
            IsMcSessionStarted = false;
            break;
        }
        case CLASS_B:
        {
            // Inform the server as soon as possible that the end-device has switched to ClassB
            LmHandlerAppData_t appData =
            {
                .Buffer = NULL,
                .BufferSize = 0,
                .Port = 0,
            };
            LmHandlerSend( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG );
            IsMcSessionStarted = true;
            break;
        }
        case CLASS_C:
        {
            IsMcSessionStarted = true;
            break;
        }
    }
}

static void OnBeaconStatusChange( LoRaMacHandlerBeaconParams_t* params )
{
    switch( params->State )
    {
        case LORAMAC_HANDLER_BEACON_RX:
        {
            puts("OnBeaconStatusChange: LORAMAC_HANDLER_BEACON_RX");
            break;
        }
        case LORAMAC_HANDLER_BEACON_LOST:
        {
            puts("OnBeaconStatusChange: LORAMAC_HANDLER_BEACON_LOST");
            break;
        }
        case LORAMAC_HANDLER_BEACON_NRX:
        {
            puts("OnBeaconStatusChange: LORAMAC_HANDLER_BEACON_NRX");
            break;
        }
        default:
        {
            break;
        }
    }

    DisplayBeaconUpdate( params );
}

#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
static void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection )
{
    IsClockSynched = isSynchronized;
}
#else
static void OnSysTimeUpdate( void )
{
    IsClockSynched = true;
}
#endif

#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static int8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size )
{
    if( size >= UNFRAGMENTED_DATA_SIZE )
    {
        return -1; // Fail
    }
    for(uint32_t i = 0; i < size; i++ )
    {
        UnfragmentedData[addr + i] = data[i];
    }
    return 0; // Success
}

static int8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size )
{
    if( size >= UNFRAGMENTED_DATA_SIZE )
    {
        return -1; // Fail
    }
    for(uint32_t i = 0; i < size; i++ )
    {
        data[i] = UnfragmentedData[addr + i];
    }
    return 0; // Success
}
#endif

static void OnFragProgress( uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost )
{
    printf( "\n###### =========== FRAG_DECODER ============ ######\n" );
    printf( "######               PROGRESS                ######\n");
    printf( "###### ===================================== ######\n");
    printf( "RECEIVED    : %5d / %5d Fragments\n", fragCounter, fragNb );
    printf( "              %5d / %5d Bytes\n", fragCounter * fragSize, fragNb * fragSize );
    printf( "LOST        :       %7d Fragments\n\n", fragNbLost );
}

#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static void OnFragDone( int32_t status, uint32_t size )
{
    FileRxCrc = Crc32( UnfragmentedData, size );
    IsFileTransferDone = true;

    printf( "\n###### =========== FRAG_DECODER ============ ######\n" );
    printf( "######               FINISHED                ######\n");
    printf( "###### ===================================== ######\n");
    printf( "STATUS      : %ld\n", status );
    printf( "CRC         : %08lX\n\n", FileRxCrc );
}
#else
static void OnFragDone( int32_t status, uint8_t *file, uint32_t size )
{
    FileRxCrc = Crc32( file, size );
    IsFileTransferDone = true;
    // Switch LED 3 OFF
    GpioWrite( &Led3, 0 );

    printf( "\n###### =========== FRAG_DECODER ============ ######\n" );
    printf( "######               FINISHED                ######\n");
    printf( "###### ===================================== ######\n");
    printf( "STATUS      : %ld\n", status );
    printf( "CRC         : %08lX\n\n", FileRxCrc );
}
#endif

static void OnTxPeriodicityChanged( uint32_t periodicity )
{
    TxPeriodicity = periodicity;

    if( TxPeriodicity == 0 )
    { // Revert to application default periodicity
        TxPeriodicity = APP_TX_DUTYCYCLE + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
    }

    // Update timer periodicity
    TimerStop( &TxTimer );
    TimerSetValue( &TxTimer, TxPeriodicity );
    TimerStart( &TxTimer );
}

static void OnTxFrameCtrlChanged( LmHandlerMsgTypes_t isTxConfirmed )
{
    LmHandlerParams.IsTxConfirmed = isTxConfirmed;
}

static void OnPingSlotPeriodicityChanged( uint8_t pingSlotPeriodicity )
{
    LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
}

///////////////////////////////////////////////////////////////////////////////
//  Event Queue

/// Test Event to be added to the Event Queue
static struct ble_npl_event event;

static void handle_event_queue(void *arg);
static void handle_event(struct ble_npl_event *ev);

/// Init the Event Queue
static void init_event_queue(void) {
    puts("init_event_queue");

    //  Init the Event Queue
    ble_npl_eventq_init(&event_queue);

    //  Init the Test Event
    ble_npl_event_init(
        &event,        //  Event
        handle_event,  //  Event Handler Function
        NULL           //  Argument to be passed to Event Handler
    );
}

/// LoRaWAN Event Loop that dequeues Events from the Event Queue and processes the Events
static void handle_event_queue(void *arg) {
    puts("handle_event_queue");

    //  Loop forever handling Events from the Event Queue
    for (;;) {
        //  Get the next Event from the Event Queue
        struct ble_npl_event *ev = ble_npl_eventq_get(
            &event_queue,         //  Event Queue
            BLE_NPL_TIME_FOREVER  //  No Timeout (Wait forever for event)
        );

        //  If no Event due to timeout, wait for next Event
        if (ev == NULL) { printf("."); continue; }
        printf("handle_event_queue: ev=%p\n", ev);

        //  Remove the Event from the Event Queue
        ble_npl_eventq_remove(&event_queue, ev);

        //  Trigger the Event Handler Function (handle_event)
        ble_npl_event_run(ev);

        // Processes the LoRaMac events
        LmHandlerProcess( );

        // If we have joined the network, do the uplink
        if (!LmHandlerIsBusy( )) {
            UplinkProcess( );
        }

        CRITICAL_SECTION_BEGIN( );
        if( IsMacProcessPending == 1 )
        {
            // Clear flag and prevent MCU to go into low power modes.
            IsMacProcessPending = 0;
        }
        else
        {
            //  The MCU wakes up through events
            //  TODO: BoardLowPowerHandler( );
        }
        CRITICAL_SECTION_END( );
    }
}

#ifdef NOTUSED
/// For Testing: Enqueue a Test Event into the Event Queue
static void put_event(char *buf, int len, int argc, char **argv) {
    puts("put_event");

    //  Add the Event to the Event Queue
    ble_npl_eventq_put(&event_queue, &event);
}
#endif  //  NOTUSED

/// For Testing: Handle a Test Event
static void handle_event(struct ble_npl_event *ev) {
    puts("handle_event");
}
