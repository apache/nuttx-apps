/****************************************************************************
 * apps/include/lte/lte_api.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_INCLUDE_LTE_LTE_API_H
#define __APPS_INCLUDE_LTE_LTE_API_H

/* - Abbreviations and terms
 *  - PDN : Packet Data Network
 *
 *      Route for transferring packets between the terminal and LTE networks.
 *
 *  - APN : Access Point Name
 *
 *      Settings required when connecting to an LTE network.
 *
 *  - IMSI : International Mobile Subscriber Identity
 *
 *      International subscriber identification number recorded
 *      on the SIM card.
 *
 *  - IMEI : International Mobile Equipment Identifier
 *
 *      International identification number assigned to
 *      data communication terminals
 *
 *  - PIN : Personal Identification Number
 *
 *  - MCC : Mobile Country Code
 *
 *      The mobile country code consists of three decimal digits.
 *
 *  - MNC : Mobile Network Code
 *
 *      The mobile network code consists of two or three decimal digits.
 *
 *  - eDRX : extended Discontinuous Reception
 *
 *      Communication technology that reduces power consumption
 *      by increasing the reception interval of various signals transmitted
 *      from LTE networks.
 *
 *  - PSM : Power Saving Mode
 *
 *      Communication technology that reduces power consumption
 *      by not communicating with the LTE network
 *      for a certain period of time.
 *
 *  - CE : Coverage Enhancement
 *
 *      Communication technology that attempts to resend data and eventually
 *      restores the original data even if the data is corrupted
 *      due to weak electric field communication.
 *
 *  - RAT : Radio Access Technology
 *
 *      Physical connection method for a radio based communication network.
 *
 * - LTE API system
 *  - Network connection API
 *
 *      Radio ON / OFF, PDN connection establishment / destruction.
 *
 *  - Communication quality and communication state API
 *
 *      Acquisition of radio status, communication status, and local time.
 *
 *  - SIM card control API
 *
 *      Get phone number / IMSI, set the PIN, get SIM status.
 *
 *  - Modem setting API
 *
 *      Get modem firmware version and IMEI. Update communication settings.
 *
 * - API call type
 *
 *      There are two types of LTE API: synchronous and asynchronous.
 *
 *  - Synchronous API
 *    - Notifies the processing result as a return value.
 *
 *    - Blocks the task that called the API until
 *      processing is completed on the modem.
 *
 *    - If the return value is -EPROTO, you can get the error code
 *      with lte_get_errinfo.
 *
 *    - If the argument attribute is out, the argument must be allocated
 *      by the caller.
 *
 *  - Asynchronous API
 *    - The processing result is notified by callback.
 *      The callback is invoked in the task context.
 *
 *    - Blocks the task that called the API until it requests
 *      processing from the modem.
 *
 *    - Notifies the processing request result as a return value.
 *
 *    - The callback is registered with the argument of each API.
 *      Registration is canceled when the processing result is notified.
 *
 *    - The same API cannot be called until the processing result is notified
 *      by the callback.(-EINPROGRESS is notified with a return value.)
 *
 *    - If the callback reports an error (LTE_RESULT_ERROR),
 *      detailed error information can be acquired with lte_get_errinfo.
 *
 *  For some APIs, both synchronous and asynchronous APIs are available.
 *  The correspondence table of API is as follows.
 *
 *
 * | Synchronous API              | Asynchronous API                  |
 * | ---------------------------- | --------------------------------- |
 * | lte_initialize               |                                   |
 * | lte_finalize                 |                                   |
 * | lte_set_report_restart       |                                   |
 * | lte_power_on                 |                                   |
 * | lte_power_off                |                                   |
 * | lte_set_report_netinfo       |                                   |
 * | lte_set_report_simstat       |                                   |
 * | lte_set_report_localtime     |                                   |
 * | lte_set_report_quality       |                                   |
 * | lte_set_report_cellinfo      |                                   |
 * | lte_get_errinfo              |                                   |
 * | lte_activate_pdn_cancel      |                                   |
 * | lte_radio_on_sync            | lte_radio_on (deprecated)         |
 * | lte_radio_off_sync           | lte_radio_off (deprecated)        |
 * | lte_activate_pdn_sync        | lte_activate_pdn                  |
 * | lte_deactivate_pdn_sync      | lte_deactivate_pdn (deprecated)   |
 * | lte_data_allow_sync          | lte_data_allow (deprecated)       |
 * | lte_get_netinfo_sync         | lte_get_netinfo (deprecated)      |
 * | lte_get_imscap_sync          | lte_get_imscap (deprecated)       |
 * | lte_get_version_sync         | lte_get_version (deprecated)      |
 * | lte_get_phoneno_sync         | lte_get_phoneno (deprecated)      |
 * | lte_get_imsi_sync            | lte_get_imsi (deprecated)         |
 * | lte_get_imei_sync            | lte_get_imei (deprecated)         |
 * | lte_get_pinset_sync          | lte_get_pinset (deprecated)       |
 * | lte_set_pinenable_sync       | lte_set_pinenable (deprecated)    |
 * | lte_change_pin_sync          | lte_change_pin (deprecated)       |
 * | lte_enter_pin_sync           | lte_enter_pin (deprecated)        |
 * | lte_get_localtime_sync       | lte_get_localtime (deprecated)    |
 * | lte_get_operator_sync        | lte_get_operator (deprecated)     |
 * | lte_get_edrx_sync            | lte_get_edrx (deprecated)         |
 * | lte_set_edrx_sync            | lte_set_edrx (deprecated)         |
 * | lte_get_psm_sync             | lte_get_psm (deprecated)          |
 * | lte_set_psm_sync             | lte_set_psm (deprecated)          |
 * | lte_get_ce_sync              | lte_get_ce (deprecated)           |
 * | lte_set_ce_sync              | lte_set_ce (deprecated)           |
 * | lte_get_siminfo_sync         | lte_get_siminfo (deprecated)      |
 * | lte_get_current_edrx_sync    | lte_get_current_edrx (deprecated) |
 * | lte_get_current_psm_sync     | lte_get_current_psm (deprecated)  |
 * | lte_get_quality_sync         | lte_get_quality (deprecated)      |
 * | lte_get_cellinfo_sync        |                                   |
 * | lte_get_rat_sync             |                                   |
 * | lte_set_rat_sync             |                                   |
 * | lte_get_ratinfo_sync         |                                   |
 * | lte_acquire_wakelock         |                                   |
 * | lte_release_wakelock         |                                   |
 * | lte_send_atcmd_sync          |                                   |
 * | lte_factory_reset_sync       |                                   |
 * | lte_set_context_save_cb      |                                   |
 * | lte_hibernation_resume       |                                   |
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <nuttx/wireless/lte/lte.h>

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Initialize resources used in LTE API.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_initialize(void);

/* Release resources used in LTE API.
 *
 * return On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_finalize(void);

/* Register the callback to notify that the modem has started up.
 *
 * The callback will be invoked if the modem starts successfully
 * after calling lte_power_on. Some APIs have to wait until
 * this callback is invoked. If no wait, those API return
 * with an error. (-ENETDOWN)
 *
 * The callback is also invoked when the modem is restarted.
 * The cause of the restart can be obtained from the callback argument.
 *
 * This function must be called after lte_initialize.
 *
 * Attention to the following
 *   when LTE_RESTART_MODEM_INITIATED is set.
 * - Asynchronous API callbacks for which results have not been
 *   notified are canceled and becomes available.
 *
 * - The processing result of the synchronous API
 *   being called results in an error. (Return value is -ENETDOWN)
 *   The errno is ENETDOWN for the socket API.
 *
 * - It should close the socket by user application.
 *
 * [in] restart_callback: Callback function to notify that
 *                        modem restarted.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_report_restart(restart_report_cb_t restart_callback);

/* Power on the modem.
 *
 * The callback which registered by lte_set_report_restart
 * will be invoked if the modem starts successfully.
 *
 * This function must be called after lte_set_report_restart.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_power_on(void);

/* Power off the modem
 *
 * Attention to the following when this API calling.
 * - For asynchronous API
 *   - callback is canceled.
 *
 * - For synchronous API
 *   - API returns with an error.
 *     - The return value is -ENETDOWN for the LTE API.
 *     - The errno is ENETDOWN for the socket API.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_power_off(void);

/* With the radio on, to start the LTE network search.
 *
 * Attention to the following when this API calling.
 * - If SIM is PIN locked, the result will be an error.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_radio_on_sync(void);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* With the radio on, to start the LTE network search.
 * The use of this API is deprecated.
 * Use lte_radio_on_sync() instead.
 *
 * Attention to the following when this API calling.
 * - If SIM is PIN locked, the result will be an error.
 *
 * [in] callback: Callback function to notify that
 *                radio on is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_radio_on(radio_on_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Exit LTE network searches with the radio off.
 *
 * If this function is called when a PDN has already been constructed,
 * the PDN is discarded.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_radio_off_sync(void);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Exit LTE network searches with the radio off.
 * The use of this API is deprecated.
 * Use lte_radio_off_sync() instead.
 *
 * If this function is called when a PDN has already been constructed,
 * the PDN is discarded.
 *
 * [in] callback: Callback function to notify that
 *                radio off is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_radio_off(radio_off_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get LTE network information.
 *
 * The maximum number of PDNs status areas must be allocated
 * before calls this API.
 *
 * [in] pdn_num: Number of pdn_stat allocated by the user.
 *               The range is from LTE_PDN_SESSIONID_MIN to
 *               LTE_PDN_SESSIONID_MAX.
 *
 * [out] info: The LTE network information.
 *             See lte_netinfo_t
 *
 * attention Immediately after successful PDN construction
 *           using lte_activate_pdn_sync() or lte_activate_pdn(),
 *           session information such as IP address
 *           may not be acquired correctly.
 *           If you want to use this API after successfully construction
 *           the PDN, wait at least 1 second before executing it.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_netinfo_sync(uint8_t pdn_num, FAR lte_netinfo_t *info);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get LTE network information.
 * The use of this API is deprecated.
 * Use lte_get_netinfo_sync() instead.
 *
 * [in] callback: Callback function to notify that
 *                get network information completed.
 *
 * attention Immediately after successful PDN construction
 *           using lte_activate_pdn_sync() or lte_activate_pdn(),
 *           session information such as IP address
 *           may not be acquired correctly.
 *           If you want to use this API after successfully construction
 *           the PDN, wait at least 1 second before executing it.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_netinfo(get_netinfo_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Constructs a PDN with the specified APN settings.
 *
 * When constructs the initial PDN,
 * LTE_APN_TYPE_IA must be set to the APN type.
 *
 * When PDN construction is successful,
 * an IP address is given from the LTE network.
 *
 * attention Attention to the following when this API calling.
 * - The initial PDN construction may take a few minutes
 *   depending on radio conditions.
 *
 * - If API is not returned, please check if the APN settings are correct.
 *
 * [in] apn: The pointer of the apn setting.
 *           See lte_apn_setting_t for valid parameters.
 *
 * [out] pdn: The construction PDN information.
 *            See lte_pdn_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 * If canceling, -ECANCELED is returned.
 */

int lte_activate_pdn_sync(FAR lte_apn_setting_t *apn, FAR lte_pdn_t *pdn);

/* Constructs a PDN with the specified APN settings.
 *
 * When constructs the initial PDN,
 * LTE_APN_TYPE_IA must be set to the APN type.
 *
 * When PDN construction is successful,
 * an IP address is given from the LTE network.
 *
 * attention Attention to the following when this API calling.
 * - The initial PDN construction may take a few minutes
 *   depending on radio conditions.
 *
 * - If the callback is not notified, please check
 *   if the APN settings are correct.
 *
 * [in] apn: The pointer of the apn setting.
 *           See lte_apn_setting_t for valid parameters.
 *
 * [in] callback: Callback function to notify that
 *                PDN activation completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_activate_pdn(FAR lte_apn_setting_t *apn, activate_pdn_cb_t callback);

/* Cancel PDN construction.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_activate_pdn_cancel(void);

/* Discard the constructed PDN.
 *
 * Discards the PDN corresponding to the session ID
 * obtained by lte_activate_pdn.
 *
 * When the discard process is successful, the IP address assigned to
 * the modem is released to the LTE network.
 *
 * [in] session_id: The numeric value of the session ID.
 *                  Use the value obtained by the lte_activate_pdn.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_deactivate_pdn_sync(uint8_t session_id);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Discard the constructed PDN.
 * The use of this API is deprecated.
 * Use lte_deactivate_pdn_sync() instead.
 *
 * Discards the PDN corresponding to the session ID
 * obtained by lte_activate_pdn.
 *
 * When the discard process is successful, the IP address assigned to
 * the modem is released to the LTE network.
 *
 * [in] session_id: The numeric value of the session ID.
 *                  Use the value obtained by the lte_activate_pdn.
 *
 * [in] callback: Callback function to notify that
 *                LTE PDN deactivation completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_deactivate_pdn(uint8_t session_id, deactivate_pdn_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Allow or disallow to data communication for specified PDN.
 *
 * attention This function is not supported.
 *
 * [in] session_id: The numeric value of the session ID.
 *                  Use the value obtained by the lte_activate_pdn.
 *
 * [in] allow: Allow or disallow to data communication for
 *             all network. Definition is as below.
 *  - LTE_DATA_ALLOW
 *  - LTE_DATA_DISALLOW
 *
 * [in] roaming_allow: Allow or disallow to data communication for
 *                     roaming network. Definition is as below.
 *  - LTE_DATA_ALLOW
 *  - LTE_DATA_DISALLOW
 *
 * -EOPNOTSUPP is returned.
 */

int lte_data_allow_sync(uint8_t session_id, uint8_t allow,
                        uint8_t roaming_allow);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Allow or disallow to data communication for specified PDN.
 * The use of this API is deprecated.
 *
 * attention This function is not supported.
 *
 * [in] session_id: The numeric value of the session ID.
 *                  Use the value obtained by the lte_activate_pdn.
 *
 * [in] allow: Allow or disallow to data communication for
 *             all network. Definition is as below.
 *  - LTE_DATA_ALLOW
 *  - LTE_DATA_DISALLOW
 *
 * [in] roaming_allow: Allow or disallow to data communication for
 *                     roaming network. Definition is as below.
 *  - LTE_DATA_ALLOW
 *  - LTE_DATA_DISALLOW
 *
 * [in] callback: Callback function to notify that
 *                configuration has changed.
 *
 * -EOPNOTSUPP is returned.
 */

int lte_data_allow(uint8_t session_id, uint8_t allow,
                   uint8_t roaming_allow, data_allow_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get whether the modem supports IMS or not.
 *
 * [out] imscap: The IMS capability.
 *               As below value stored.
 *  - LTE_ENABLE
 *  - LTE_DISABLE
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_imscap_sync(FAR bool *imscap);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get whether the modem supports IMS or not.
 * The use of this API is deprecated.
 * Use lte_get_imscap_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting IMS capability is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_imscap(get_imscap_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Acquires the FW version information of the modem.
 *
 * [out] version: The version information of the modem.
 *                See lte_version_t
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_version_sync(FAR lte_version_t *version);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Acquires the FW version information of the modem.
 * The use of this API is deprecated.
 * Use lte_get_version_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting the version is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_version(get_ver_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get phone number from SIM.
 *
 * [out] phoneno: A character string indicating phone number.
 *                It is terminated with '\0'.
 *                The maximum number of phone number areas
 *                must be allocated. See LTE_PHONENO_LEN.
 * [in] len:   Length of the buffer for storing phone number.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_phoneno_sync(FAR char *phoneno);
#else
int lte_get_phoneno_sync(FAR char *phoneno, size_t len);
#endif

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get phone number from SIM.
 * The use of this API is deprecated.
 * Use lte_get_phoneno_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting the phone number is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_phoneno(get_phoneno_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get International Mobile Subscriber Identity from SIM.
 *
 * [out] imsi: A character string indicating IMSI.
 *             It is terminated with '\0'.
 *             The maximum number of IMSI areas
 *             must be allocated. See LTE_IMSI_LEN.
 * [in] len:   Length of the buffer for storing IMSI.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_imsi_sync(FAR char *imsi);
#else
int lte_get_imsi_sync(FAR char *imsi, size_t len);
#endif

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get International Mobile Subscriber Identity from SIM.
 * The use of this API is deprecated.
 * Use lte_get_imsi_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting IMSI is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_imsi(get_imsi_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get International Mobile Equipment Identifier from the modem.
 *
 * [out] imei: A character string indicating IMEI.
 *             It is terminated with '\0'.
 *             The maximum number of IMEI areas
 *             must be allocated. See LTE_IMEI_LEN.
 * [in] len:   Length of the buffer for storing IMEI.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_imei_sync(FAR char *imei);
#else
int lte_get_imei_sync(FAR char *imei, size_t len);
#endif

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get International Mobile Equipment Identifier from the modem.
 * The use of this API is deprecated.
 * Use lte_get_imei_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting IMEI is completed
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_imei(get_imei_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get Personal Identification Number settings.
 *
 * [out] pinset: PIN settings information.
 *               See lte_getpin_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_pinset_sync(FAR lte_getpin_t *pinset);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get Personal Identification Number settings.
 * The use of this API is deprecated.
 * Use lte_get_pinset_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting the PIN setting is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_pinset(get_pinset_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Set Personal Identification Number enable.
 *
 * [in] enable: "Enable" or "Disable".
 *              Definition is as below.
 *  - LTE_ENABLE
 *  - LTE_DISABLE
 *
 * [in] pincode: Current PIN code. Minimum number of digits is 4.
 *               Maximum number of digits is 8, end with '\0'.
 *               (i.e. Max 9 byte)
 *
 * [out] attemptsleft: Number of attempts left.
 *                     Set only if failed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_pinenable_sync(bool enable, FAR char *pincode,
                           FAR uint8_t *attemptsleft);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Set Personal Identification Number enable.
 * The use of this API is deprecated.
 * Use lte_set_pinenable_sync() instead.
 *
 * [in] enable: "Enable" or "Disable".
 *              Definition is as below.
 *  - LTE_ENABLE
 *  - LTE_DISABLE
 *
 * [in] pincode: Current PIN code. Minimum number of digits is 4.
 *               Maximum number of digits is 8, end with '\0'.
 *               (i.e. Max 9 byte)
 *
 * [in] callback: Callback function to notify that
 *                setting of PIN enables/disables is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_pinenable(bool enable, FAR char *pincode,
                      set_pinenable_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Change Personal Identification Number.
 *
 * It can be changed only when PIN is enable.
 *
 * [in] target_pin: Target of change PIN.
 *                  Definition is as below.
 *  - LTE_TARGET_PIN
 *  - LTE_TARGET_PIN2
 *
 * [in] pincode: Current PIN code. Minimum number of digits is 4.
 *               Maximum number of digits is 8, end with '\0'.
 *               (i.e. Max 9 byte)
 *
 * [in] new_pincode: New PIN code. Minimum number of digits is 4.
 *                   Maximum number of digits is 8, end with '\0'.
 *                   (i.e. Max 9 byte)
 *
 * [out] attemptsleft: Number of attempts left.
 *                     Set only if failed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_change_pin_sync(int8_t target_pin, FAR char *pincode,
                        FAR char *new_pincode, FAR uint8_t *attemptsleft);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Change Personal Identification Number.
 * The use of this API is deprecated.
 * Use lte_change_pin_sync() instead.
 *
 * It can be changed only when PIN is enable.
 *
 * [in] target_pin: Target of change PIN.
 *                  Definition is as below.
 *  - LTE_TARGET_PIN
 *  - LTE_TARGET_PIN2
 *
 * [in] pincode: Current PIN code. Minimum number of digits is 4.
 *               Maximum number of digits is 8, end with '\0'.
 *               (i.e. Max 9 byte)
 *
 * [in] new_pincode: New PIN code. Minimum number of digits is 4.
 *                   Maximum number of digits is 8, end with '\0'.
 *                   (i.e. Max 9 byte)
 *
 * [in] callback: Callback function to notify that
 *                change of PIN is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_change_pin(int8_t target_pin, FAR char *pincode,
                   FAR char *new_pincode, change_pin_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Enter Personal Identification Number.
 *
 * [in] pincode: Current PIN code. Minimum number of digits is 4.
 *               Maximum number of digits is 8, end with '\0'.
 *               (i.e. Max 9 byte)
 *
 * [in] new_pincode: Always set NULL.
 *                   This parameter is not currently used.
 *                   If this parameter has a value in it,
 *                   this API will error.
 *
 * [out] simstat: State after PIN enter.
 *                As below value stored.
 * - LTE_PINSTAT_READY
 * - LTE_PINSTAT_SIM_PIN
 * - LTE_PINSTAT_SIM_PUK
 * - LTE_PINSTAT_PH_SIM_PIN
 * - LTE_PINSTAT_PH_FSIM_PIN
 * - LTE_PINSTAT_PH_FSIM_PUK
 * - LTE_PINSTAT_SIM_PIN2
 * - LTE_PINSTAT_SIM_PUK2
 * - LTE_PINSTAT_PH_NET_PIN
 * - LTE_PINSTAT_PH_NET_PUK
 * - LTE_PINSTAT_PH_NETSUB_PIN
 * - LTE_PINSTAT_PH_NETSUB_PUK
 * - LTE_PINSTAT_PH_SP_PIN
 * - LTE_PINSTAT_PH_SP_PUK
 * - LTE_PINSTAT_PH_CORP_PIN
 * - LTE_PINSTAT_PH_CORP_PUK
 *
 * [out] attemptsleft: Number of attempts left.
 *                     Set only if failed.
 *                     If simstat is other than PIN, PUK, PIN2, PUK2,
 *                     set the number of PIN.
 *
 * note Running this API when the SIM state is
 *      other than LTE_PINSTAT_SIM_PIN will return an error.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 *
 * deprecated This API will be removed in a future version
 */

int lte_enter_pin_sync(FAR char *pincode, FAR char *new_pincode,
                       FAR uint8_t *simstat, FAR uint8_t *attemptsleft);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Enter Personal Identification Number.
 * The use of this API is deprecated.
 * Use lte_enter_pin_sync() instead.
 *
 * [in] pincode: Current PIN code. Minimum number of digits is 4.
 *               Maximum number of digits is 8, end with '\0'.
 *               (i.e. Max 9 byte)
 *
 * [in] new_pincode: Always set NULL.
 *                   This parameter is not currently used.
 *                   If this parameter has a value in it,
 *                   this API will error.
 *
 * [in] callback: Callback function to notify that
 *                PIN enter is completed.
 *
 * note Running this API when the SIM state is
 *      other than LTE_PINSTAT_SIM_PIN will return an error.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 *
 * deprecated This API will be removed in a future version
 */

int lte_enter_pin(FAR char *pincode, FAR char *new_pincode,
                  enter_pin_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get local time.
 *
 * [out] localtime: Local time. See lte_localtime_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_localtime_sync(FAR lte_localtime_t *localtime);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get local time.
 * The use of this API is deprecated.
 * Use lte_get_localtime_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting local time is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_localtime(get_localtime_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get connected network operator information.
 *
 * [out] oper: A character string indicating network operator.
 *             It is terminated with '\0' If it is not connected,
 *             the first character is '\0'.
 *             The maximum number of network operator areas
 *             must be allocated. See LTE_OPERATOR_LEN.
 * [in] len:   Length of the buffer for storing network operator.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_operator_sync(FAR char *oper);
#else
int lte_get_operator_sync(FAR char *oper, size_t len);
#endif

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get connected network operator information.
 * The use of this API is deprecated.
 * Use lte_get_operator_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting network operator information is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_operator(get_operator_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get eDRX settings.
 *
 * [out] settings: eDRX settings. See lte_edrx_setting_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_edrx_sync(FAR lte_edrx_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get eDRX settings.
 * The use of this API is deprecated.
 * Use lte_get_edrx_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting eDRX settings are completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_edrx(get_edrx_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Set eDRX settings.
 *
 * [in] settings: eDRX settings.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_edrx_sync(FAR lte_edrx_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Set eDRX settings.
 * The use of this API is deprecated.
 * Use lte_set_edrx_sync() instead.
 *
 * [in] settings: eDRX settings.
 *
 * [in] callback: Callback function to notify that
 *                eDRX settings are completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_edrx(FAR lte_edrx_setting_t *settings, set_edrx_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get PSM settings.
 *
 * [out] settings: PSM settings. See lte_psm_setting_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_psm_sync(FAR lte_psm_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get PSM settings.
 * The use of this API is deprecated.
 * Use lte_get_psm_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting PSM settings are completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_psm(get_psm_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Set PSM settings.
 *
 * [in] settings: PSM settings.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_psm_sync(FAR lte_psm_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Set PSM settings.
 * The use of this API is deprecated.
 * Use lte_set_psm_sync() instead.
 *
 * [in] settings: PSM settings.
 *
 * [in] callback: Callback function to notify that
 *                PSM settings are completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_psm(FAR lte_psm_setting_t *settings, set_psm_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get CE settings.
 *
 * [out] settings: CE settings. See lte_ce_setting_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_ce_sync(FAR lte_ce_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get CE settings.
 * The use of this API is deprecated.
 * Use lte_get_ce_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting CE settings are completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_ce(get_ce_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Set CE settings.
 *
 * [in] settings: CE settings
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_ce_sync(FAR lte_ce_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Set CE settings.
 * The use of this API is deprecated.
 * Use lte_set_ce_sync() instead.
 *
 * [in] settings: CE settings
 *
 * [in] callback: Callback function to notify that
 *                CE settings are completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_ce(FAR lte_ce_setting_t *settings, set_ce_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Notifies the SIM status to the application.
 *
 * The default report setting is disable.
 *
 * [in] simstat_callback: Callback function to notify that SIM state.
 *                        If NULL is set,
 *                        the report setting is disabled.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_report_simstat(simstat_report_cb_t simstat_callback);

/* Notifies the Local time to the application.
 *
 * The default report setting is disable.
 *
 * [in] localtime_callback: Callback function to notify that
 *                          local time. If NULL is set,
 *                          the report setting is disabled.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_report_localtime(localtime_report_cb_t localtime_callback);

/* Notifies the communication quality information to the application.
 *
 * Invoke the callback at the specified report interval.
 *
 * The default report setting is disable.
 *
 * attention When changing the notification cycle, stop and start again.
 *
 * [in] quality_callback: Callback function to notify that
 *                        quality information. If NULL is set,
 *                        the report setting is disabled.
 *
 * [in] period: Reporting cycle in sec (1-4233600)
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_report_quality(quality_report_cb_t quality_callback,
                           uint32_t period);

/* Notifies the LTE network cell information to the application.
 *
 * Invoke the callback at the specified report interval.
 *
 * The default report setting is disable.
 *
 * attention When changing the notification cycle, stop and start again.
 *
 * [in] cellinfo_callback: Callback function to notify that
 *                         cell information. If NULL is set,
 *                         the report setting is disabled.
 *
 * [in] period: Reporting cycle in sec (1-4233600)
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_report_cellinfo(cellinfo_report_cb_t cellinfo_callback,
                            uint32_t period);

/* Notifies the LTE network status to the application.
 *
 * The default report setting is disable.
 *
 * [in] netinfo_callback: Callback function to notify that
 *                        cell information. If NULL is set,
 *                        the report setting is disabled.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_report_netinfo(netinfo_report_cb_t netinfo_callback);

/* Get LTE API last error information.
 *
 * Call this function when LTE_RESULT_ERROR is returned by
 * callback function. The detailed error information can be obtained.
 *
 * [in] info: Pointer of error information.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_errinfo(FAR lte_errinfo_t *info);

/* Get SIM information such as Mobile Country Code/Mobile Network Code.
 *
 * [in] option:   Indicates which parameter to get.
 *                Bit setting definition is as below.
 *                 - LTE_SIMINFO_GETOPT_MCCMNC
 *                 - LTE_SIMINFO_GETOPT_SPN
 *                 - LTE_SIMINFO_GETOPT_ICCID
 *                 - LTE_SIMINFO_GETOPT_IMSI
 *                 - LTE_SIMINFO_GETOPT_GID1
 *                 - LTE_SIMINFO_GETOPT_GID2
 *
 * [out] siminfo: SIM information. See lte_siminfo_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_siminfo_sync(uint32_t option, FAR lte_siminfo_t *siminfo);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get SIM information such as Mobile Country Code/Mobile Network Code.
 * The use of this API is deprecated.
 * Use lte_get_siminfo_sync() instead.
 *
 * [in] option:   Indicates which parameter to get.
 *                Bit setting definition is as below.
 *                - LTE_SIMINFO_GETOPT_MCCMNC
 *                - LTE_SIMINFO_GETOPT_SPN
 *                - LTE_SIMINFO_GETOPT_ICCID
 *                - LTE_SIMINFO_GETOPT_IMSI
 *                - LTE_SIMINFO_GETOPT_GID1
 *                - LTE_SIMINFO_GETOPT_GID2
 *
 * [in] callback: Callback function to notify that
 *                get of SIM information is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_siminfo(uint32_t option, get_siminfo_cb_t callback);

/* Get eDRX dynamic parameter.
 *
 * deprecated Use lte_get_current_edrx instead.
 *
 * This API can be issued after connect to the LTE network
 * with lte_activate_pdn().
 *
 * [in] callback: Callback function to notify when
 *                getting eDRX dynamic parameter is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_dynamic_edrx_param(get_dynamic_edrx_param_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get current eDRX settings.
 *
 * This API can be issued after connect to the LTE network
 * with lte_activate_pdn().
 *
 * Get the settings negotiated between the modem and the network.
 *
 * [out] settings: Current eDRX settings.
 *                 See lte_edrx_setting_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_current_edrx_sync(FAR lte_edrx_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get current eDRX settings.
 * The use of this API is deprecated.
 * Use lte_get_current_edrx_sync() instead.
 *
 * This API can be issued after connect to the LTE network
 * with lte_activate_pdn().
 *
 * Get the settings negotiated between the modem and the network.
 *
 * [in] callback: Callback function to notify when
 *                getting current eDRX settings is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_current_edrx(get_current_edrx_cb_t callback);

/* Get PSM dynamic parameter.
 *
 * deprecated Use lte_get_current_psm instead.
 *
 * This API can be issued after connect to the LTE network
 * with lte_activate_pdn().
 *
 * [in] callback: Callback function to notify when
 *                getting PSM dynamic parameter is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_dynamic_psm_param(get_dynamic_psm_param_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get current PSM settings.
 *
 * This API can be issued after connect to the LTE network
 * with lte_activate_pdn().
 *
 * Get the settings negotiated between the modem and the network.
 *
 * [OUT] settings: Current PSM settings.
 *                 See lte_psm_setting_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_current_psm_sync(FAR lte_psm_setting_t *settings);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get current PSM settings.
 * The use of this API is deprecated.
 * Use lte_get_current_psm_sync() instead.
 *
 * This API can be issued after connect to the LTE network
 * with lte_activate_pdn().
 *
 * Get the settings negotiated between the modem and the network.
 *
 * [in] callback: Callback function to notify when
 *                getting current PSM settings is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_current_psm(get_current_psm_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get communication quality information.
 *
 * [out] quality: Quality information. See lte_quality_t
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_quality_sync(FAR lte_quality_t *quality);

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

/* Get communication quality information.
 * The use of this API is deprecated.
 * Use lte_get_quality_sync() instead.
 *
 * [in] callback: Callback function to notify when
 *                getting quality information is completed.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_quality(get_quality_cb_t callback);

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

/* Get LTE network cell information.
 *
 * attention This function is not supported yet.
 *
 * [out] cellinfo: LTE network cell information.
 *                 See lte_cellinfo_t
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_cellinfo_sync(FAR lte_cellinfo_t *cellinfo);

/* Get RAT type
 *
 * On success, RAT type shown below is returned.
 * - LTE_RAT_CATM
 * - LTE_RAT_NBIOT
 * On failure, negative value is returned according to <errno.h>.
 */

int lte_get_rat_sync(void);

/* Set RAT setting
 *
 * [in] rat: RAT type. Definition is as below.
 *           - LTE_RAT_CATM
 *           - LTE_RAT_NBIOT
 * [in] persistent: Flag to keep RAT settings
 *                  after power off the modem.
 *                  Definition is as below.
 *                  - LTE_ENABLE
 *                  - LTE_DISABLE
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_rat_sync(uint8_t rat, bool persistent);

/* Get RAT information
 *
 * [out] info: Pointer to the structure that
 *             stores RAT information
 *             See lte_ratinfo_t.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_ratinfo_sync(FAR lte_ratinfo_t *info);

/* Acquire the modem wakelock. If any wakelock is acquired, modem can't
 * enter to the sleep state.
 * Please call this API after calling lte_initialize().
 * Otherwise this API will result in an error.
 * Before calling lte_finalize(), must release all wakelocks
 * acquired by this API.
 *
 * On success, return the count of the current modem wakelock. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_acquire_wakelock(void);

/* Release the modem wakelock. If all of the wakelock are released,
 * modem can enter to the sleep state.
 * Please call this API after calling lte_initialize().
 * Otherwise this API will result in an error.
 *
 * On success, return the count of the current modem wakelock. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_release_wakelock(void);

/* Get the number of wakelock counts acquired.
 * Please call this API after calling lte_initialize().
 * Otherwise this API will result in an error.
 *
 * On success, return the count of the current modem wakelock. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_get_wakelock_count(void);

/* Send AT command to the modem.
 *
 * [in] cmd: The AT command data.
 *           Maximum length is LTE_AT_COMMAND_MAX_LEN.
 *           AT command is shall begin with "AT" and end with '\r'.
 * [in] cmdlen: Length of the AT command data.
 * [in] respbuff: The area to store the AT command response.
 * [in] respbufflen: Length of the AT command response buffer.
 * [in] resplen: The pointer to the area store
 *               the length of AT command response.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_send_atcmd_sync(FAR const char *cmd, int cmdlen,
                        FAR char *respbuff, int respbufflen,
                        FAR int *resplen);

/* Run factory reset on the modem.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_factory_reset_sync(void);

/* Set callback function for context save.
 *
 * [in] callback: Callback function to notify a context data
 *                when modem entering hibernation mode.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_set_context_save_cb(context_save_cb_t callback);

/* Resume LTE status from hibernation mode.
 *
 * [in] res_ctx: Context data for resume daemon.
 *
 * [in] len    : Context data size.
 *
 * On success, 0 is returned. On failure,
 * negative value is returned according to <errno.h>.
 */

int lte_hibernation_resume(FAR const uint8_t *res_ctx, int len);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_LTE_LTE_API_H */
