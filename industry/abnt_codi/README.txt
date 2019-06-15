The ABNT CODI is an old energy meter standard used in Brazil.

This code interprets the end user serial output existent in the energy meter.
That output externalizes its data blinking an LED as a serial protocol at the
baudrate of 110 BPS and uses 8 octects:
 _____________________________________________________________________________
|        |      |                                                             |
| OCTECT | bits | Description                                                 |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  001   | 0-7  | Number of seconds to the end of current active demand (LSB) |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  002   | 0-3  | Number of seconds to the end of current active demand (MSB) |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   4  | Bill indicator. It is inverted at each demand replenishment |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   5  | Reactive Interval Indicator. Inverted at end react interval |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   6  | If 1 means the reactive-capacitive is used to calculate UFER|
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   7  | If 1 means the reactive-inductive is used to calculate UFER |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  003   | 0-3  | Current seasonal segment:                                   |
|        |      | 0001 - tip                                                  |
|        |      | 0010 - out of tip                                           |
|        |      | 1000 - reserved                                             |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        | 4-5  | Type of charging indicator (flag):                          |
|        |      | 00 - Blue                                                   |
|        |      | 01 - Green                                                  |
|        |      | 10 - Irrigators                                             |
|        |      | 11 - Other                                                  |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   6  | Not used                                                    |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   7  | If equal 1 means reactive rate is enabled                   |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  004   | 0-7  | Number of pulses for active energy of cur dem interv (LSB)  |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  005   | 0-6  | Number of pulses for active energy of cur dem interv (MSB)  |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   7  | Not used                                                    |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  006   | 0-7  | Number of pulses for reactive energy of cur dem interv (LSB)|
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  007   | 0-6  | Number of pulses for reactive energy of cur dem interv (MSB)|
|________|______|_____________________________________________________________|
|        |      |                                                             |
|        |   7  | Not used                                                    |
|________|______|_____________________________________________________________|
|        |      |                                                             |
|  008   | 0-7  | Inverted bits of "xor" from previous octects                |
|________|______|_____________________________________________________________|

