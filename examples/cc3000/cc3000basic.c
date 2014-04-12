/***************************************************************************
 * apps/examples/cc3000basic.c
 *
 * Derives from an application to demo an Arduino connected to the TI CC3000
 *
 *   Copyright (C) 2013 Chris Magagna - cmagagna@yahoo.com
 *   Port to nuttx:
 *      Alan Carvalho de Assis <acassis@gmail.com>
 *      David Sidrane <david_s5@nscdg.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Don't sue me if my code blows up your board and burns down your house
 *
 ****************************************************************************
 *
 * To connect an Arduino to the CC3000 you'll need to make these 6 connections
 * (in addition to the WiFi antenna, power etc).
 *
 * Name / pin on CC3000 module / pin on CC3000EM board / purpose
 *
 * SPI_CS     / 12 / J4-8 /  SPI Chip Select
 *                           The Arduino will set this pin LOW when it wants to
 *                           exchange data with the CC3000. By convention this is
 *                           Arduino pin 10, but any pin can be used. In this
 *                           program it will be called WLAN_CS
 *
 * SPI_DOUT   / 13 / J4-9 /  Data from the module to the Arduino
 *                           This is Arduino's MISO pin, and is how the CC3000
 *                           will get bytes to the Arduino. For most Arduinos
 *                           MISO is pin 12
 *
 * SPI_IRQ    / 14 / J4-10 / CC3000 host notify
 *                           The CC3000 will drive this pin LOW to let the Arduino
 *                           know it's ready to send data. For a regular Arduino
 *                           (Uno, Nano, Leonardo) this will have to be connected
 *                           to pin 2 or 3 so you can use attachInterrupt(). In
 *                           this program it will be called WLAN_IRQ
 *
 * SPI_DIN    / 15 / J4-11   Data from the Arduino to the CC3000
 *                           This is the Arduino's MOSI pin, and is how the Arduino
 *                           will get bytes to the CC3000. For most Arduinos
 *                           MOSI is pin 11
 *
 * SPI_CLK    / 17 / J4-12   SPI clock
 *                           This is the Arduino's SCK pin. For most Arduinos
 *                           SCK is pin 13
 *
 * VBAT_SW_EN / 26 / J5-5    Module enable
 *                           The Arduino will set this pin HIGH to turn the CC3000
 *                           on. Any pin can be used. In this program it will be
 *                           called WLAN_EN
 *
 * WARNING #1: The CC3000 runs at 3.6V maximum so you can't run it from your
 * regular 5V Arduino power pin. Run it from 3.3V!
 *
 * WARNING #2: When transmitting the CC3000 will use up to 275mA current. Most
 * Arduinos' 3.3V pins can only supply up to 50mA current, so you'll need a
 * separate power supply for it (or a voltage regulator like the LD1117V33
 * connected to your Arduino's 5V power pin).
 *
 * WARNING #3: The CC3000's IO pins are not 5V tolerant. If you're using a 5V
 * Arduino you will need a level shifter to convert these signals to 3.3V
 * so you don't blow up the module.
 *
 * You'll need to shift the pins for WLAN_CS, MOSI, SCK, and WLAN_EN. MISO can be
 * connected directly because it's an input pin for the Arduino and the Arduino
 * can read 3.3V signals directly. For WLAN_IRQ use a pullup resistor of 20K to
 * 100K Ohm -- one leg to the Arduino input pin + CC3000 SPI_IRQ pin, the other
 * leg to +3.3V.
 *
 * You can use a level shifter chip like the 74LVC245 or TXB0104 or you can use
 * a pair of resistors to make a voltage divider like this:
 *
 * Arduino pin -----> 560 Ohm --+--> 1K Ohm -----> GND
 *                              |
 *                              |
 *                              +---> CC3000 pin
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
/*
 * Memory Analyses
 *
 *              total       used       free    largest
 * Mem:         16560      11144       5416       5384
 * PID   SIZE   USED   THREAD NAME
 *     0      0      0 Idle Task
 *     1    876    772 init
 *     2    604    588 c3b
 *     3    236    220 <pthread0>
 *
 *     8    364    348 <pthread0>
 *
 *     9    260    196 <pthread>
 *    10    380    364 Telnet dd
 *    11    860    844 Telnet sd
 */

 #include <nuttx/config.h>

#include "board.h"
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <nuttx/arch.h>
#include <nuttx/wireless/cc3000/nvmem.h>
#include <nuttx/wireless/cc3000/include/sys/socket.h>
#include <nuttx/wireless/cc3000/wlan.h>
#include <nuttx/wireless/cc3000/hci.h>
#include <nuttx/wireless/cc3000/security.h>
#include <nuttx/wireless/cc3000/netapp.h>
#include "shell.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void Initialize(void);
void helpme(void);
int execute(int cmd);
void ShowBufferSize(void);
void StartSmartConfig(void);
void ManualConnect(void);
void ManualAddProfile(void);
void ListAccessPoints(void);
void PrintIPBytes(uint8_t *ipBytes);
void ShowInformation(void);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MS_PER_SEC 1000
#define US_PER_MS  1000
#define US_PER_SEC 1000000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t isInitialized = false;

#ifdef CONFIG_EXAMPLES_CC3000_MEM_CHECK
static struct mallinfo mmstart;
static struct mallinfo mmprevious;
#endif

/****************************************************************************
 *  Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_CC3000_MEM_CHECK
static void show_memory_usage(struct mallinfo *mmbefore,
                              struct mallinfo *mmafter)
{
  int diff;

  printf("              total       used       free    largest\n");
  printf("Before:%11d%11d%11d%11d\n",
             mmbefore->arena, mmbefore->uordblks, mmbefore->fordblks, mmbefore->mxordblk);
  printf("After: %11d%11d%11d%11d\n",
             mmafter->arena, mmafter->uordblks, mmafter->fordblks, mmafter->mxordblk);

  diff = mmbefore->uordblks - mmafter->uordblks;
  if (diff < 0)
    {
      printf("Change:%11d allocated\n", -diff);
    }
  else if (diff > 0)
    {
      printf("Change:%11d freed\n", diff);
    }

#ifdef CONFIG_EXAMPLES_CC3000_STACK_CHECK
  stkmon_disp();
#endif
}
#endif

#ifdef CONFIG_EXAMPLES_CC3000_STACK_CHECK
static char buff[CONFIG_TASK_NAME_SIZE+1];
static void _stkmon_disp(FAR struct tcb_s *tcb, FAR void *arg)
{
#if CONFIG_TASK_NAME_SIZE > 0
  strncpy(buff,tcb->name,CONFIG_TASK_NAME_SIZE);
  buff[CONFIG_TASK_NAME_SIZE] = '\0';
  syslog("%5d %6d %6d %s\n",
         tcb->pid, tcb->adj_stack_size, up_check_tcbstack(tcb), buff);
#else
  syslog("%5d %6d %6d\n",
         tcb->pid, tcb->adj_stack_size, up_check_tcbstack(tcb));
#endif
}
#endif

static bool wait(long timeoutMs, volatile unsigned long *what,
                 volatile unsigned long is)
{
  long t_ms;
  struct timeval end, start;

  gettimeofday(&start, NULL);

  while (*what != is)
    {
      usleep(10*US_PER_MS);
      gettimeofday(&end, NULL);
      t_ms = ((end.tv_sec - start.tv_sec) * MS_PER_SEC) + ((end.tv_usec - start.tv_usec) / US_PER_MS) ;
      if (t_ms > timeoutMs)
        {
        return false;
        }
    }

  return true;
}

static bool wait_on(long timeoutMs, volatile unsigned long *what,
                    volatile unsigned long is, char * msg)
{
  printf(msg);
  printf("...");
  fflush(stdout);
  bool ret = wait(timeoutMs,what,is);
  if (!ret)
    {
      printf(" FAILED:Timeout!\n");
    }
  else
    {
      printf(" Succeed\n");
    }

  fflush(stdout);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_CC3000_STACK_CHECK
#  define stkmon_disp()
#else
void stkmon_disp(void)
{
#if CONFIG_TASK_NAME_SIZE > 0
  syslog("%-5s %-6s %-6s %s\n", "PID", "SIZE", "USED", "THREAD NAME");
#else
  syslog("%-5s %-6s %-6s\n", "PID", "SIZE", "USED");
#endif
  sched_foreach(_stkmon_disp, NULL);
}
#endif

void AsyncEventPrint(void)
{
  printf("\n");
  switch(lastAsyncEvent)
    {
      printf("CC3000 Async event: Simple config done\n");
      break;

    case HCI_EVNT_WLAN_UNSOL_CONNECT:
      printf("CC3000 Async event: Unsolicited connect\n");
      break;

    case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
      printf("CC3000 Async event: Unsolicted disconnect\n");
      break;

    case HCI_EVNT_WLAN_UNSOL_DHCP:
      printf("CC3000 Async event: Got IP address via DHCP: ");
      printf("%d", dhcpIPAddress[0]);
      printf(".");
      printf("%d", dhcpIPAddress[1]);
      printf(".");
      printf("%d", dhcpIPAddress[2]);
      printf(".");
      printf("%d\n", dhcpIPAddress[3]);
      break;

    case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
      printf("CC3000 Async event: OK to shut down\n");
      break;

    case HCI_EVNT_WLAN_KEEPALIVE:
      /* Once initialized, the CC3000 will send these keepalive events
       * every 20 seconds.
       */

      printf("CC3000 Async event: Keepalive\n");
      return;
      break;

    default:
      printf("AsyncCallback called with unhandled event! (0x%X)\n", lastAsyncEvent);
      break;
    }
}

void helpme(void)
{
  printf("\n+-------------------------------------------+\n");
  printf("|      Nuttx CC3000 Demo Program            |\n");
  printf("+-------------------------------------------+\n\n");
  printf("  01 - Initialize the CC3000\n");
  printf("  02 - Show RX & TX buffer sizes, & free RAM\n");
  printf("  03 - Start Smart Config\n");
  printf("  04 - Manually connect to AP\n");
  printf("  05 - Manually add connection profile\n");
  printf("  06 - List access points\n");
  printf("  07 - Show CC3000 information\n");
  printf("  08 - Telnet\n");
  printf("\n Type 01-07 to select above option: ");
}

int execute(int cmd)
{
  int ret = 0;
  if (asyncNotificationWaiting)
    {
      asyncNotificationWaiting = false;
      AsyncEventPrint();
    }

  printf("\n");
  switch(cmd)
    {
    case '1':
      Initialize();
      break;

    case '2':
      ShowBufferSize();
      break;

    case '3':
      StartSmartConfig();
      break;

    case '4':
      ManualConnect();
      break;

    case '5':
      ManualAddProfile();
      break;

    case '6':
      ListAccessPoints();
      break;

    case '7':
      ShowInformation();
      break;

    case '8':
     if (!isInitialized)
       {
         Initialize();
       }

#ifdef CONFIG_EXAMPLES_CC3000_MEM_CHECK
      mmprevious= mallinfo();
      show_memory_usage(&mmstart,&mmprevious);
#endif
      shell_main(0, 0);
#ifdef CONFIG_EXAMPLES_CC3000_MEM_CHECK
      mmprevious= mallinfo();
      show_memory_usage(&mmstart,&mmprevious);
#endif
      break;

    case 'q':
    case 'Q':
      ret = 1;
      break;

    default:
      printf("**Unknown command \"%d\" **\n", cmd);
      break;
    }

    return ret;
}

void Initialize(void)
{
#ifdef CONFIG_EXAMPLES_CC3000_MEM_CHECK
  mmstart = mallinfo();
  memcpy(&mmprevious, &mmstart, sizeof(struct mallinfo));
  show_memory_usage(&mmstart,&mmprevious);
#endif

  uint8_t fancyBuffer[MAC_ADDR_LEN];

  if (isInitialized)
    {
      printf("CC3000 already initialized. Shutting down and restarting...\n");
      wlan_stop();
      usleep(1000000); /* Delay 1s */
    }

  printf("Initializing CC3000...\n");
  CC3000_Init();
#ifdef CONFIG_EXAMPLES_CC3000_STACK_CHECK
  stkmon_disp();
#endif
  printf("  CC3000 init complete.\n");

  if (nvmem_read_sp_version(fancyBuffer) == 0)
    {
      printf("  Firmware version is: ");
      printf("%d", fancyBuffer[0]);
      printf(".");
      printf("%d\n", fancyBuffer[1]);
    }
  else
    {
      printf("Unable to get firmware version. Can't continue.\n");
      return;
    }

#if 0
  if (nvmem_get_mac_address(fancyBuffer) == 0)
    {
      printf("  MAC address: ");
      for (i = 0; i < MAC_ADDR_LEN; i++)
        {
          if (i != 0)
            {
              printf(":");
            }
          printf("%X", fancyBuffer[i]);
        }

      printf("\n");
      isInitialized = true;
    }
  else
    {
      printf("Unable to get MAC address. Can't continue.\n");
    }
#else
    isInitialized = true;
#endif

#ifdef CONFIG_EXAMPLES_CC3000_MEM_CHECK
    mmprevious = mallinfo();
    show_memory_usage(&mmstart,&mmprevious);
#endif
}

/* This just shows the compiled size of the transmit & recieve buffers */

void ShowBufferSize(void)
{
  printf("Transmit buffer is %d bytes", CC3000_TX_BUFFER_SIZE);
  printf("Receive buffer is %d bytes", CC3000_RX_BUFFER_SIZE);
}

/* Smart Config is TI's way to let you connect your device to your WiFi network
 * without needing a keyboard and display to enter the network name, password,
 * etc. You run a little app on your iPhone, Android device, or laptop with Java
 * and it sends the config info to the CC3000 automagically, so the end user
 * doesn't need to do anything complicated. More details here:
 *
 *   http://processors.wiki.ti.com/index.php/CC3000_Smart_Config
 *
 * This example deletes any currently saved WiFi profiles and goes over the top
 * with error checking, so it's easier to see exactly what's going on. You
 * probably won't need all of this code for your own Smart Config implementation.
 *
 * This example also doesn't use any of the AES enhanced security setup API calls
 * because frankly they're weirder than I want to deal with.
 */

/* The Simple Config Prefix always needs to be 'TTT' */

char simpleConfigPrefix[] = {'T', 'T', 'T'};

/* This is the default Device Name that TI's Smart Config app for iPhone etc. use.
 * You can change it to whatever you want, but then your users will need to type
 * that name into their phone or tablet when they run Smart Config.
 */

char device_name[]  = "CC3000";

void StartSmartConfig(void)
{
  long rval;

  if (!isInitialized)
    {
      printf("CC3000 not initialized; can't run Smart Config.\n");
      return;
    }

  printf("Starting Smart Config\n");

  printf("  Disabling auto-connect policy...");
  if ((rval = wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE)) !=0 )
    {
      printf(" Failed!\n    Setting auto connection policy failed, error: %X\n", rval);
      return;
    }

  printf(" Succeed\n");
  printf("  Deleting all existing profiles...");
  fflush(stdout);

  if ((rval = wlan_ioctl_del_profile(255)) !=0 )
    {
      printf(" Failed!\n    Deleting all profiles failed, error: %X\n", rval);
      return;
    }

  printf(" Succeed\n");
  wait_on(20*MS_PER_SEC, &ulCC3000Connected, 0, "  Waiting until disconnected");

  printf("  Setting smart config prefix...");
  fflush(stdout);

  if ((rval = wlan_smart_config_set_prefix(simpleConfigPrefix)) !=0 )
    {
      printf(" Failed!\n    Setting smart config prefix failed, error: %X", rval);
      return;
    }

  printf(" Succeed\n");
  printf("  Starting smart config...");
  fflush(stdout);

  if ((rval = wlan_smart_config_start(0)) !=0 )
    {
      printf(" Failed!\n    Starting smart config failed, error: %X\n", rval);
      return;
    }

  printf(" Succeed\n");

  if (!wait_on(30*MS_PER_SEC, &ulSmartConfigFinished, 1, "  Waiting on Starting smart config done"))
    {
      printf("    Timed out waiting for Smart Config to finish. Hopefully it did anyway\n");
    }

  printf("  Smart Config packet %s!\n",ulSmartConfigFinished ? "seen" : "NOT seen");

  printf("  Enabling auto-connect policy...");
  fflush(stdout);
  if ((rval=wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE)) !=0 )
    {
      printf(" Failed!\n    Setting auto connection policy failed, error: %X\n", rval);
      return;
    }

  printf(" Succeed\n");
  printf("  Stopping CC3000...\n");
  fflush(stdout);
  wlan_stop();  /* No error returned here, so nothing to check */

  printf("  Pausing for 2 seconds...\n");
  usleep(2000000);

  printf("  Restarting CC3000... \n");
  wlan_start(0);  /* No error returned here, so nothing to check */

  if (!wait_on(20*MS_PER_SEC, &ulCC3000Connected, 1, "  Waiting for connection to AP"))
    {
      printf("    Timed out waiting for connection to AP\n");
      return;
    }

  if (!wait_on(15*MS_PER_SEC, &ulCC3000DHCP, 1, "  Waiting for IP address from DHCP"))
    {
      printf("    Timed out waiting for IP address from DHCP\n");
      return;
    }

  printf("  Sending mDNS broadcast to signal we're done with Smart Config...\n");
  fflush(stdout);

  /* The API documentation says mdnsAdvertiser() is supposed to return 0 on
   * success and SOC_ERROR on failure, but it looks like what it actually
   * returns is the socket number it used. So we ignore it.
   */

  mdnsadvertiser(1, device_name, strlen(device_name));

  printf("  Smart Config finished Successfully!\n");
  ShowInformation();
  fflush(stdout);
}

/* This is an example of how you'd connect the CC3000 to an AP without using
 * Smart Config or a stored profile.
 *
 * All the code above wlan_connect() is just for this demo program; if you're
 * always going to connect to your network this way you wouldn't need it.
 */

void ManualConnect(void)
{
  char ssidName[] = "YourAP";
  char AP_KEY[] = "yourpass";
  uint8_t rval;

  if (!isInitialized)
    {
      printf("CC3000 not initialized; can't run manual connect.\n");
      return;
    }

  printf("Starting manual connect...\n");

  printf("  Disabling auto-connect policy...\n");
  (void)wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);

  printf("  Deleting all existing profiles...\n");
  (void)wlan_ioctl_del_profile(255);

  wait_on(15*MS_PER_SEC, &ulCC3000Connected, 0, "    Waiting until disconnected");

  printf("  Manually connecting...\n");

  /* Parameter 1 is the security type: WLAN_SEC_UNSEC, WLAN_SEC_WEP,
   *        WLAN_SEC_WPA or WLAN_SEC_WPA2
   * Parameter 3 is the MAC adddress of the AP. All the TI examples
   *        use NULL. I suppose you would want to specify this
   *        if you were security paranoid.
   */

  rval = wlan_connect(WLAN_SEC_WPA2,
        ssidName,
        strlen(ssidName),
        NULL,
        (uint8_t *)AP_KEY,
        strlen(AP_KEY));

  if (rval == 0)
    {
      printf("  Manual connect success.\n");
    }
  else
    {
      printf("  Unusual return value: %d\n", rval);
    }
}

/* This is an example of manually adding a WLAN profile to the CC3000. See
 * wlan_ioctl_set_connection_policy() for more details of how profiles are
 * used but basically there's 7 slots where you can store AP info and if
 * the connection policy is set to auto_start then the CC3000 will go
 * through its profile table and try to auto-connect to something it knows
 * about after it boots up.
 *
 * Note the API documentation for wlan_add_profile is wrong. It says it
 * returns 0 on success and -1 on failure. What it really returns is
 * the stored profile number (0-6, since the CC3000 can store 7) or
 * 255 on failure.
 *
 * Unfortunately the API doesn't give you any way to see how many profiles
 * are in use or which profile is stored in which slot, so if you want to
 * manage multiple profiles you'll need to do that yourself.
 */

void ManualAddProfile(void)
{
  char ssidName[] = "YourAP";
  char AP_KEY[] = "yourpass";
  uint8_t rval;

  if (!isInitialized)
    {
      printf("CC3000 not initialized; can't run manual add profile.");
      return;
    }

  printf("Starting manual add profile...\n");

  printf("  Disabling auto connection...\n");
  wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);

  printf("  Adding profile...\n");
  rval = wlan_add_profile  (
          WLAN_SEC_WPA2,     /* WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2 */
          (uint8_t *)ssidName,
          strlen(ssidName),
          NULL,              /* BSSID, TI always uses NULL */
          0,                 /* Profile priority */
          0x18,              /* Key length for WEP security, undocumented why this needs to be 0x18 */
          0x1e,              /* Key index, undocumented why this needs to be 0x1e */
          0x2,               /* key management, undocumented why this needs to be 2 */
          (uint8_t *)AP_KEY, /* WPA security key */
          strlen(AP_KEY)     /* WPA security key length */
          );

  if (rval!=255)
    {
      /* This code is lifted from http://e2e.ti.com/support/low_power_rf/f/851/p/180859/672551.aspx;
       * the actual API documentation on wlan_add_profile doesn't specify any of this....
       */

      printf("  Manual add profile success, stored in profile: %d\n", rval);

      printf("  Enabling auto connection...\n");
      wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

      printf("  Stopping CC3000...\n");
      wlan_stop();

      printf("  Stopping for 5 seconds...\n");
      usleep(5000000);

      printf("  Restarting CC3000...\n");
      wlan_start(0);

      printf("  Manual add profile done!");
    }
  else
    {
      printf("  Manual add profile failured (all profiles full?).");
    }
}

/* The call wlan_ioctl_get_scan_results returns this structure. I couldn't
 * find it in the TI library so it's defined here. It's 50 bytes with
 * a semi weird arrangement but fortunately it's not as bad as it looks.
 *
 * numNetworksFound - 4 bytes - On the first call to wlan_ioctl_get_scan_results
 *         this will be set to how many APs the CC3000 sees. Although
 *         with 4 bytes the CC3000 could see 4 billion APs in my testing
 *         this number was always 20 or less so there's probably an
 *         internal memory limit.
 *
 * results - 4 bytes - 0=aged results, 1=results valid, 2=no results. Why TI
 *         used 32 bits to store something that could be done in 2,
 *         and how this field is different than isValid below, is
 *         a mystery to me so I just igore this field completely.
 *
 * isValid & rssi - 1 byte - a packed structure. The top bit (isValid)
 *         indicates whether or not this structure has valid data,
 *         the bottom 7 bits (rssi) are the signal strength of this AP.
 *
 * securityMode & ssidLength - 1 byte - another packed structure. The top 2
 *         bits (securityMode) show how the AP is configured:
 *           0 - open / no security
 *           1 - WEP
 *           2 - WPA
 *           3 - WPA2
 *         ssidLength is the lower 6 bytes and shows how many characters
 *         (up to 32) of the ssid_name field are valid
 *
 * frameTime - 2 bytes - how long, in seconds, since the CC3000 saw this AP
 *         beacon
 *
 * ssid_name - 32 bytes - The ssid name for this AP. Note that this isn't a
 *         regular null-terminated C string so you can't use it
 *         directly with a strcpy() or Serial.println() etc. and you'll
 *         need a 33-byte string to store it (32 valid characters +
 *         null terminator)
 *
 * bssid - 6 bytes - the MAC address of this AP
 */

typedef struct scanResults
{
  unsigned long numNetworksFound;
  unsigned long results;
  unsigned isValid:1;
  unsigned rssi:7;
  unsigned securityMode:2;
  unsigned ssidLength:6;
  uint16_t frameTime;
  uint8_t ssid_name[32];
  uint8_t bssid[6];
} scanResults;

#define NUM_CHANNELS  16

void ListAccessPoints(void)
{
  unsigned long aiIntervalList[NUM_CHANNELS];
  uint8_t rval;
  scanResults sr;
  int apCounter, i;
  char localB[33];

  if (!isInitialized)
    {
      printf("CC3000 not initialized; can't list access points.\n");
      return;
    }

  printf("List visible access points\n");

  printf("  Setting scan paramters...\n");

  for (i=0; i<NUM_CHANNELS; i++)
    {
      aiIntervalList[i] = 2000;
    }

  rval = wlan_ioctl_set_scan_params(
      1000,           /* Enable start application scan */
      100,            /* Minimum dwell time on each channel */
      100,            /* Maximum dwell time on each channel */
      5,              /* Number of probe requests */
      0x7ff,          /* Channel mask */
      -80,            /* RSSI threshold */
      0,              /* SNR threshold */
      205,            /* Probe TX power */
      aiIntervalList  /* Table of scan intervals per channel */
      );

  if (rval!=0)
    {
      printf("  Got back unusual result from wlan_ioctl_set_scan_params, can't continue: %d\n", rval);
      return;
    }

#if 0
  printf("  Sleeping 5 seconds to let the CC3000 discover APs...\n");
  usleep(5000000);
#endif

  printf("  Getting AP count...\n");

  /* On the first call to get_scan_results, sr.numNetworksFound will return the
   * actual # of APs currently seen. Get that # then loop through and print
   * out what's found.
   */

  if ((rval=wlan_ioctl_get_scan_results(2000, (uint8_t *)&sr))!=0)
    {
      printf("  Got back unusual result from wlan_ioctl_get scan results, can't continue: %d\n", rval);
      return;
    }

  apCounter = sr.numNetworksFound;
  printf("  Number of APs found: %d\n", apCounter);

  do
    {
      if (sr.isValid)
        {
          printf("    ");
          switch(sr.securityMode)
            {
            case WLAN_SEC_UNSEC:  /* 0 */
              printf("OPEN ");
              break;
            case WLAN_SEC_WEP:    /* 1 */
              printf("WEP  ");
              break;
            case WLAN_SEC_WPA:    /* 2 */
              printf("WPA  ");
              break;
            case WLAN_SEC_WPA2:   /* 3 */
              printf("WPA2 ");
              break;
            }

          sprintf(localB, "%3d  ", sr.rssi);
          printf("%s", localB);
          memset(localB, 0, 33);
          memcpy(localB, sr.ssid_name, sr.ssidLength);
          printf("%s\n", localB);
        }

      if (--apCounter>0)
        {
          if ((rval=wlan_ioctl_get_scan_results(2000, (uint8_t *)&sr)) !=0 )
            {
              printf("  Got back unusual result from wlan_ioctl_get scan, can't continue: %d\n", rval);
              return;
            }
        }
    }
  while (apCounter>0);

  printf("  Access Point list finished.\n");
}

void PrintIPBytes(uint8_t *ipBytes)
{
  printf("%d.%d.%d.%d\n", ipBytes[3], ipBytes[2], ipBytes[1], ipBytes[0]);
}

/* All the data in all the fields from netapp_ipconfig() are reversed,
 * e.g. an IP address is read via bytes 3,2,1,0 instead of bytes
 * 0,1,2,3 and the MAC address is read via bytes 5,4,3,2,1,0 instead
 * of 0,1,2,3,4,5.
 *
 * N.B. TI is inconsistent here; nvmem_get_mac_address() returns them in
 * the right order etc.
 */

void ShowInformation(void)
{
  tNetappIpconfigRetArgs inf;
  char localB[33];
  int i;

  if (!isInitialized)
    {
      printf("CC3000 not initialized; can't get information.\n");
      return;
    }

  printf("CC3000 information:\n");

  netapp_ipconfig(&inf);

  printf("  IP address: ");
  PrintIPBytes(inf.aucIP);

  printf("  Subnet mask: ");
  PrintIPBytes(inf.aucSubnetMask);

  printf("  Gateway: ");
  PrintIPBytes(inf.aucDefaultGateway);

  printf("  DHCP server: ");
  PrintIPBytes(inf.aucDHCPServer);

  printf("  DNS server: ");
  PrintIPBytes(inf.aucDNSServer);

  printf("  MAC address: ");
  for (i=(MAC_ADDR_LEN-1); i>=0; i--)
    {
      if (i!=(MAC_ADDR_LEN-1))
        {
        printf(":");
      }
      printf("%X", inf.uaMacAddr[i]);
    }

  printf("\n");

  memset(localB, 0, sizeof(localB));
  strncpy(localB, (char*)inf.uaSSID,sizeof(localB));

  printf("  Connected to SSID: %s\n", localB);
}

int c3b_main(int argc, char *argv[])
{
  char ch='0';

  do
    {
      helpme();
      stkmon_disp();

      ch = getchar();

    }
  while (execute(ch) == 0);

  return 0;
}
