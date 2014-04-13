/**************************************************************************
*
*  This file is part of the ArduinoCC3000 library.

*  Version 1.0.1b
*
*  Copyright (C) 2013 Chris Magagna - cmagagna@yahoo.com
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  Don't sue me if my code blows up your board and burns down your house
*
*
*  This file is the main module for the Arduino CC3000 library.
*  Your program must call CC3000_Init() before any other API calls.
*
****************************************************************************/

/*
	Some things are different for the Teensy 3.0, so set a flag if we're using
	that hardware.
*/

#include <stdint.h>

#if defined(__arm__) && defined(CORE_TEENSY) && defined(__MK20DX128__)
#define TEENSY3   1
#endif






/* I used the Teensy 3.0 to get the Arduino CC3000 library working but the
   Teensy's hardware SPI and the CC3000's SPI didn't like each other so I had
   to send the bits manually. For the Uno, Nano, etc. you can probably leave
   this unchanged. If your Arduino can't talk to the CC3000 and you're sure
   your wiring is OK then try changing this. */

#ifdef TEENSY3
#define USE_HARDWARE_SPI	false
#else
#define USE_HARDWARE_SPI	true
#endif









// These are the Arduino pins that connect to the CC3000
// (in addition to standard SPI pins MOSI, MISO, and SCK)
//
// The WLAN_IRQ pin must be supported by attachInterrupt
// on your platform

#ifndef TEENSY3

#define WLAN_CS			10		// Arduino pin connected to CC3000 WLAN_SPI_CS
#define WLAN_EN			9		// Arduino pin connected to CC3000 VBAT_SW_EN
#define WLAN_IRQ		3		// Arduino pin connected to CC3000 WLAN_SPI_IRQ
#define WLAN_IRQ_INTNUM	1		// The attachInterrupt() number that corresponds
                                // to WLAN_IRQ
#define WLAN_MOSI		MOSI
#define WLAN_MISO		MISO
#define WLAN_SCK		SCK
#else

#define WLAN_CS			25
#define WLAN_MISO		26
#define WLAN_IRQ		27
#define WLAN_IRQ_INTNUM	27		// On the Teensy 3.0 the interrupt # is the same as the pin #
#define WLAN_MOSI		28
#define WLAN_SCK		29
#define WLAN_EN			30

#endif











/*
	The timing between setting the CS pin and reading the IRQ pin is very
	tight on the CC3000, and sometimes the default Arduino digitalRead()
	and digitalWrite() functions are just too slow.
	
	For many of the CC3000 library functions this isn't a big deal because the
	IRQ pin is tied to an interrupt routine but some of them of them disable
	the interrupt routine and read the pins directly. Because digitalRead()
	/ Write() are so slow once in a while the Arduino will be in the middle of
	its pin code and the CC3000 will flip another pin's state and it will be
	missed, and everything locks up.
	
	The upshot of all of this is we need to read & write the pin states
	directly, which is very fast compared to the built in Arduino functions.
	
	The Teensy 3.0's library has built in macros called digitalReadFast()
	& digitalWriteFast() that compile down to direct port manipulations but
	are still readable, so use those if possible.
	
	There's a digitalReadFast() / digitalWriteFast() library for Arduino but
	it looks like it hasn't been updated since 2010 so I think it's best to
	just use the direct port manipulations.
*/

#ifdef TEENSY3

#define Read_CC3000_IRQ_Pin()			digitalReadFast(WLAN_IRQ)
#define Set_CC3000_CS_NotActive()		digitalWriteFast(WLAN_CS, HIGH)
#define Set_CC3000_CS_Active()			digitalWriteFast(WLAN_CS, LOW)

#else

// This is hardcoded for an ATMega328 and pin 3. You will need to change this
// for other MCUs or pins
#define Read_CC3000_IRQ_Pin()			((PIND & B00001000) ? 1 : 0)

// This is hardcoded for an ATMega328 and pin 10. You will need to change this
// for other MCUs or pins
#define Set_CC3000_CS_NotActive()		PORTB |= B00000100
#define Set_CC3000_CS_Active()			PORTB &= B11111011

#endif



















#define MAC_ADDR_LEN	6



#define DISABLE	(0)

#define ENABLE	(1)

//AES key "smartconfigAES16"
//const uint8_t smartconfigkey[] = {0x73,0x6d,0x61,0x72,0x74,0x63,0x6f,0x6e,0x66,0x69,0x67,0x41,0x45,0x53,0x31,0x36};





/* If you uncomment the line below the library will leave out a lot of the
   higher level functions but use a lot less memory. From:

   http://processors.wiki.ti.com/index.php/Tiny_Driver_Support

  CC3000's new driver has flexible memory compile options.

  This feature comes in handy when we want to use a limited RAM size MCU.

  Using The Tiny Driver Compilation option will create a tiny version of our
  host driver with lower data, stack and code consumption.

  By enabling this feature, host driver's RAM consumption can be reduced to
  minimum of 251 bytes.

  The Tiny host driver version will limit the host driver API to the most
  essential ones.

  Code size depends on actual APIs used.

  RAM size depends on the largest packet sent and received.

  CC3000 can now be used with ultra low cost MCUs, consuming 251 byte of RAM
  and 2K to 6K byte of code size, depending on the API usage. */

//#define CC3000_TINY_DRIVER	1





extern uint8_t asyncNotificationWaiting;
extern long lastAsyncEvent;
extern uint8_t dhcpIPAddress[];



extern void CC3000_Init(void);


extern volatile unsigned long ulSmartConfigFinished,
	ulCC3000Connected,
	ulCC3000DHCP,
	OkToDoShutDown,
	ulCC3000DHCP_configured;

extern volatile uint8_t ucStopSmartConfig;
