/****************************************************************************
 * apps/examples/sx1262_test/sx1262_test_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

//  Demo Program for LoRa SX1262 on NuttX
#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../../../nuttx/libs/libsx1262/include/radio.h"
#include "../../../nuttx/libs/libsx1262/include/sx126x-board.h"

/// TODO: We are using LoRa Frequency 923 MHz for Singapore. Change this for your region.
#define USE_BAND_923

#if defined(USE_BAND_433)
    #define RF_FREQUENCY               434000000 /* Hz */
#elif defined(USE_BAND_780)
    #define RF_FREQUENCY               780000000 /* Hz */
#elif defined(USE_BAND_868)
    #define RF_FREQUENCY               868000000 /* Hz */
#elif defined(USE_BAND_915)
    #define RF_FREQUENCY               915000000 /* Hz */
#elif defined(USE_BAND_923)
    #define RF_FREQUENCY               923000000 /* Hz */
#else
    #error "Please define a frequency band in the compiler options."
#endif

/// LoRa Parameters
#define LORAPING_TX_OUTPUT_POWER            14        /* dBm */

#define LORAPING_BANDWIDTH                  0         /* [0: 125 kHz, */
                                                      /*  1: 250 kHz, */
                                                      /*  2: 500 kHz, */
                                                      /*  3: Reserved] */
#define LORAPING_SPREADING_FACTOR           7         /* [SF7..SF12] */
#define LORAPING_CODINGRATE                 1         /* [1: 4/5, */
                                                      /*  2: 4/6, */
                                                      /*  3: 4/7, */
                                                      /*  4: 4/8] */
#define LORAPING_PREAMBLE_LENGTH            8         /* Same for Tx and Rx */
#define LORAPING_SYMBOL_TIMEOUT             5         /* Symbols */
#define LORAPING_FIX_LENGTH_PAYLOAD_ON      false
#define LORAPING_IQ_INVERSION_ON            false

#define LORAPING_TX_TIMEOUT_MS              3000    /* ms */
#define LORAPING_RX_TIMEOUT_MS              10000    /* ms */
#define LORAPING_BUFFER_SIZE                64      /* LoRa message size */

const uint8_t loraping_ping_msg[] = "PING";  //  We send a "PING" message
const uint8_t loraping_pong_msg[] = "PONG";  //  We expect a "PONG" response

static uint8_t loraping_buffer[LORAPING_BUFFER_SIZE];  //  64-byte buffer for our LoRa messages
static int loraping_rx_size;

/// LoRa Statistics
struct {
    int rx_timeout;
    int rx_ping;
    int rx_pong;
    int rx_other;
    int rx_error;
    int tx_timeout;
    int tx_success;
} loraping_stats;

///////////////////////////////////////////////////////////////////////////////
//  Main Function

static void read_registers(void);
static void init_driver(void);
static void send_message(void);
static void receive_message(void);
static void create_task(void);

int main(int argc, FAR char *argv[]) {

    /* Call SX1262 Library */

    test_libsx1262();

//  Uncomment to read SX1262 registers
//  #define READ_REGISTERS
// #ifdef READ_REGISTERS
    //  Read SX1262 registers 0x00 to 0x0F
    read_registers();
// #endif  //  READ_REGISTERS

    //  TODO: Create a Background Thread to handle LoRa Events
    create_task();

    //  Init SX1262 driver
    init_driver();

    //  TODO: Do we need to wait?
    sleep(1);

//  Uncomment to send a LoRa message
//  #define SEND_MESSAGE
#ifdef SEND_MESSAGE
    //  Send a LoRa message
    send_message();
#endif  //  SEND_MESSAGE

//  Uncomment to receive a LoRa message
#define RECEIVE_MESSAGE
#ifdef RECEIVE_MESSAGE
    //  Handle LoRa events for the next 10 seconds
    for (int i = 0; i < 10; i++) {
        //  Prepare to receive a LoRa message
        receive_message();

        //  Process the received LoRa message, if any
        RadioOnDioIrq(NULL);

        //  Sleep for 1 second
        usleep(1000 * 1000);
    }
#endif  //  RECEIVE_MESSAGE

    //  TODO: Close the SPI Bus
    //  close(spi);
    puts("Done!");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  LoRa Commands

static void send_once(int is_ping);
static void on_tx_done(void);
static void on_rx_done(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
static void on_tx_timeout(void);
static void on_rx_timeout(void);
static void on_rx_error(void);

/// Read SX1262 registers
static void read_registers(void) {
    puts("read_registers");

    //  Init the SPI port
    SX126xIoInit();

    //  Read and print the first 16 registers: 0 to 15
    for (uint16_t addr = 0; addr < 0x10; addr++) {
        //  Read the register
        uint8_t val = SX126xReadRegister(addr);      //  For SX1262
        //  uint8_t val = SX1276Read(addr);          //  For SX1276

        //  Print the register value
        printf("Register 0x%02x = 0x%02x\n", addr, val);
    }
}

/// Initialise the SX1262 driver.
/// Assume that create_task has been called to init the Event Queue.
static void init_driver(void) {
    puts("init_driver");

    //  Set the LoRa Callback Functions
    RadioEvents_t radio_events;
    memset(&radio_events, 0, sizeof(radio_events));  //  Must init radio_events to null, because radio_events lives on stack!
    radio_events.TxDone    = on_tx_done;
    radio_events.RxDone    = on_rx_done;
    radio_events.TxTimeout = on_tx_timeout;
    radio_events.RxTimeout = on_rx_timeout;
    radio_events.RxError   = on_rx_error;

    //  Init the SPI Port and the LoRa Transceiver
    Radio.Init(&radio_events);

    //  Set the LoRa Frequency
    Radio.SetChannel(RF_FREQUENCY);

    //  Configure the LoRa Transceiver for transmitting messages
    Radio.SetTxConfig(
        MODEM_LORA,
        LORAPING_TX_OUTPUT_POWER,
        0,        //  Frequency deviation: Unused with LoRa
        LORAPING_BANDWIDTH,
        LORAPING_SPREADING_FACTOR,
        LORAPING_CODINGRATE,
        LORAPING_PREAMBLE_LENGTH,
        LORAPING_FIX_LENGTH_PAYLOAD_ON,
        true,     //  CRC enabled
        0,        //  Frequency hopping disabled
        0,        //  Hop period: N/A
        LORAPING_IQ_INVERSION_ON,
        LORAPING_TX_TIMEOUT_MS
    );

    //  Configure the LoRa Transceiver for receiving messages
    Radio.SetRxConfig(
        MODEM_LORA,
        LORAPING_BANDWIDTH,
        LORAPING_SPREADING_FACTOR,
        LORAPING_CODINGRATE,
        0,        //  AFC bandwidth: Unused with LoRa
        LORAPING_PREAMBLE_LENGTH,
        LORAPING_SYMBOL_TIMEOUT,
        LORAPING_FIX_LENGTH_PAYLOAD_ON,
        0,        //  Fixed payload length: N/A
        true,     //  CRC enabled
        0,        //  Frequency hopping disabled
        0,        //  Hop period: N/A
        LORAPING_IQ_INVERSION_ON,
        true      //  Continuous receive mode
    );
}

/// Send a LoRa message. Assume that SX1262 driver has been initialised.
static void send_message(void) {
    puts("send_message");

    //  Send the "PING" message
    send_once(1);
}

/// Send a LoRa message. If is_ping is 0, send "PONG". Otherwise send "PING".
static void send_once(int is_ping) {
    //  Copy the "PING" or "PONG" message to the transmit buffer
    if (is_ping) {
        memcpy(loraping_buffer, loraping_ping_msg, 4);
    } else {
        memcpy(loraping_buffer, loraping_pong_msg, 4);
    }

    //  Fill up the remaining space in the transmit buffer (64 bytes) with values 0, 1, 2, ...
    for (int i = 4; i < sizeof loraping_buffer; i++) {
        loraping_buffer[i] = i - 4;
    }

    //  We send the transmit buffer (64 bytes)
    Radio.Send(loraping_buffer, sizeof loraping_buffer);
}

/// Receive a LoRa message. Assume that SX1262 driver has been initialised.
/// Assume that create_task has been called to init the Event Queue.
static void receive_message(void) {
    puts("receive_message");

    //  Receive a LoRa message within the timeout period
    Radio.Rx(LORAPING_RX_TIMEOUT_MS);
}

///////////////////////////////////////////////////////////////////////////////
//  LoRa Callback Functions

/// Callback Function that is called when our LoRa message has been transmitted
static void on_tx_done(void)
{
    puts("Tx done");

    //  Log the success status
    loraping_stats.tx_success++;

    //  Switch the LoRa Transceiver to low power, sleep mode
    Radio.Sleep();

    //  TODO: Receive a "PING" or "PONG" LoRa message
    //  os_eventq_put(os_eventq_dflt_get(), &loraping_ev_rx);
}

/// Callback Function that is called when a LoRa message has been received
static void on_rx_done(
    uint8_t *payload,  //  Buffer containing received LoRa message
    uint16_t size,     //  Size of the LoRa message
    int16_t rssi,      //  Signal strength
    int8_t snr)        //  Signal To Noise ratio
{
    puts("Rx done:");

    //  Switch the LoRa Transceiver to low power, sleep mode
    Radio.Sleep();

    //  Copy the received packet
    if (size > sizeof loraping_buffer) {
        size = sizeof loraping_buffer;
    }
    loraping_rx_size = size;
    memcpy(loraping_buffer, payload, size);

    //  Log the signal strength, signal to noise ratio
    //  TODO: loraping_rxinfo_rxed(rssi, snr);

    //  Dump the contents of the received packet
    for (int i = 0; i < loraping_rx_size; i++) {
        printf("%02x ", loraping_buffer[i]);
    }
    puts("");

    //  TODO: Send a "PING" or "PONG" LoRa message
    //  os_eventq_put(os_eventq_dflt_get(), &loraping_ev_tx);
}

/// Callback Function that is called when our LoRa message couldn't be transmitted due to timeout
static void on_tx_timeout(void) {
    puts("Tx timeout");

    //  Switch the LoRa Transceiver to low power, sleep mode
    Radio.Sleep();

    //  Log the timeout
    loraping_stats.tx_timeout++;

    //  TODO: Receive a "PING" or "PONG" LoRa message
    //  os_eventq_put(os_eventq_dflt_get(), &loraping_ev_rx);
}

/// Callback Function that is called when no LoRa messages could be received due to timeout
static void on_rx_timeout(void) {
    puts("Rx timeout");

    //  Switch the LoRa Transceiver to low power, sleep mode
    Radio.Sleep();

    //  Log the timeout
    loraping_stats.rx_timeout++;
    //  TODO: loraping_rxinfo_timeout();

    //  TODO: Send a "PING" or "PONG" LoRa message
    //  os_eventq_put(os_eventq_dflt_get(), &loraping_ev_tx);
}

/// Callback Function that is called when we couldn't receive a LoRa message due to error
static void on_rx_error(void) {
    puts("Rx error");

    //  Log the error
    loraping_stats.rx_error++;

    //  Switch the LoRa Transceiver to low power, sleep mode
    Radio.Sleep();

    //  TODO: Send a "PING" or "PONG" LoRa message
    //  os_eventq_put(os_eventq_dflt_get(), &loraping_ev_tx);
}

///////////////////////////////////////////////////////////////////////////////
//  Multitasking Commands

/// Event Queue containing Events to be processed
////TODO struct ble_npl_eventq event_queue;

/// Event to be added to the Event Queue
////TODO struct ble_npl_event event;

static void task_callback(void *arg);
static void handle_event(struct ble_npl_event *ev);

/// TODO: Create a Background Task to handle LoRa Events
/// This is unused because we don't have a Background Thread to process the Event Queue.
static void create_task(void) {
    puts("TODO: create_task");

#ifdef TODO
    //  Init the Event Queue
    ble_npl_eventq_init(&event_queue);

    //  Init the Event
    ble_npl_event_init(
        &event,        //  Event
        handle_event,  //  Event Handler Function
        NULL           //  Argument to be passed to Event Handler
    );
#endif  //  TODO

    //  TODO: Create a Background Thread to process the Event Queue
    //  nimble_port_freertos_init(task_callback);
}

/// TODO: Enqueue an Event into the Event Queue.
/// This is unused because we don't have a Background Thread to process the Event Queue.
static void put_event(char *buf, int len, int argc, char **argv) {
    puts("TODO: put_event");

#ifdef TODO
    //  Add the Event to the Event Queue
    ble_npl_eventq_put(&event_queue, &event);
#endif  //  TODO
}

/// TODO: Task Function that dequeues Events from the Event Queue and processes the Events.
/// This is unused because we don't have a Background Thread to process the Event Queue.
static void task_callback(void *arg) {
    puts("TODO: task_callback");

#ifdef TODO
    //  Loop forever handling Events from the Event Queue
    for (;;) {
        //  Get the next Event from the Event Queue
        struct ble_npl_event *ev = ble_npl_eventq_get(
            &event_queue,  //  Event Queue
            1000           //  Timeout in 1,000 ticks
        );

        //  If no Event due to timeout, wait for next Event
        if (ev == NULL) { continue; }

        //  Remove the Event from the Event Queue
        ble_npl_eventq_remove(&event_queue, ev);

        //  Trigger the Event Handler Function (handle_event)
        ble_npl_event_run(ev);
    }
#endif  //  TODO
}

/// TODO: Handle an Event
/// This is unused because we don't have a Background Thread to process the Event Queue.
static void handle_event(struct ble_npl_event *ev) {
    puts("handle_event");
    puts("Handle an event");
}