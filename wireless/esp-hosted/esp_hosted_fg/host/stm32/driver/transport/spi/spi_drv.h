// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/** prevent recursive inclusion **/
#ifndef __SPI_DRV_H
#define __SPI_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

/** Includes **/
#include "common.h"
#include "transport_drv.h"

/** constants/macros **/

/* NSS or CS0 configuration (Pin 11) */
/* In case of different board than STM32F469I,
 * User need to update SPI NSS pin as per hardware*/
#ifndef USR_SPI_CS_GPIO_Port
#define USR_SPI_CS_GPIO_Port    GPIOA
#endif
#ifndef USR_SPI_CS_Pin
#define USR_SPI_CS_Pin          GPIO_PIN_15
#endif

/** Exported Structures **/

/** Exported variables **/

/** Inline functions **/

/** Exported Functions **/

#ifdef __cplusplus
}
#endif

#endif
