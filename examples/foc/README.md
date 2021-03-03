# FOC example

The main purpose of this example is to provide a universal template to
implement the motor controller based on the kernel-side FOC device and
the application-side FOC library.

At the moment, this example implements a simple open-loop velocity controller.

# Hardware setup

This example has not yet implemented any mechanism to protect the
powered device. This means that there is no overtemeprature
protection, no overcurrent protection and no overvoltage protection.

Make sure that you power the device properly and provide current
limits on your own so as not to break your hardware.

# Configuration

The FOC PI current controller parameters can be obtained from the given 
equations:

```
Kp = ccb * Ls;
pp = Rs / Ls;
Ki = pp * Kp * T;
```

where:
  Kp  - PI proportional coefficient
  Ki  - PI integral coefficient
  Rs  - average phase serial resistance
  Ls  - average phase serial inductance
  pp  - pole plant
  ccb - current control bandwidth
  T   - sampling period

## Sample parameters for some commercially available motors

* Odrive D6374 150KV
    p      = 7
    Rs     = 0.0254 Ohm
    Ls     = 8.73 uH
    i\_max = ?
    v\_max = ?
 
  Example configuration for f\_PWM = 20kHz, f\_notifier = 10kHz, ccb=1000:
    Kp = 0.0087
    Ki = 0.0025
 
* Linix 45ZWN24-40 (PMSM motor dedicated for NXP FRDM-MC-LVMTR kit)
    p      = 2
    Rs     = 0.5 Ohm
    Ls     = 0.400 mH
    i\_max = 2.34 A
    v\_max = 24 V
 
  Example configuration for f\_PWM = 10kHz, f\_notifier = 5kHz, ccb=1000:
    Kp = 0.4
    Ki = 0.1
 
* Bull-Running BR2804-1700 kV (motor provided with the ST P-NUCLEO-IHM07 kit)
    p      = 7
    Rs     = 0.11 Ohm
    Ls     = 0.018 mH
    i\_max = 1.2A
    v\_max = 12V
 
  Example configuration for f\_PWM = 20kHz, f\_notifier = 10kHz, ccb=200:
    Kp = 0.036
    Ki = 0.022
 
* iPower GBM2804H-100T (gimbal motor provided with the ST P-NUCLEO-IHM03 kit)
    p      = 7
    Rs     = 5.29 Ohm
    Ls     = 1.05 mH
    i\_max = 0.15A
    v\_max = 12V
 
  Example configuration for f\_PWM = 10kHz, f\_notifier = 5kHz, ccb=TODO:
    Kp = TODO
    Ki = TODO
 
