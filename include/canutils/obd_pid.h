/****************************************************************************
 * apps/include/canutils/obd_pid.h
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

#ifndef __APPS_INCLUDE_CANUTILS_OBD_PID_H
#define __APPS_INCLUDE_CANUTILS_OBD_PID_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* PID REQUEST */

#define OBD_PID_STD_REQUEST             0x7df       /* Standard PID REQUEST Message ID = 0x7df or 0x7e0 */
#define OBD_PID_EXT_REQUEST             0x18db33f1  /* Extended PID REQUEST Messaged ID = 0x18db33f1 */

/* PID RESPONSE */

#define OBD_PID_STD_RESPONSE            0x7e8       /* Standard PID RESPONSE Message ID = 0x7e8 */
#define OBD_PID_EXT_RESPONSE            0x18daf110  /* Extended PID RESPONSE Message ID = 0x18daf111 or 0x18daf11d */

#define OBD_RESP_BASE                   0x40        /* Response mode = (0x40 + OpMode) */

/* OBD Operation Modes */

#define OBD_SHOW_DATA                   0x01 /* Used to read current data from vehicle */
#define OBD_SHOW_FREEZED_DATA           0x02 /* Used to read freezed data from vehicle */
#define OBD_SHOW_DTC                    0x03 /* Show Diagnostic Trouble Codes */
#define OBD_CLEAR_DTC                   0x04 /* Clear Diagnostic Trouble Codes stored in the vehicle */
#define OBD_TEST_RESULT1                0x05 /* Test Results */
#define OBD_TEST_RESULT2                0x06 /* Test Results */
#define OBD_SHOW_PEND_DTC               0x07 /* Show Pending Diagnostic Trouble Codes */
#define OBD_CONTROL_OPERATION           0x08 /* Control Operation of on-board component/system */
#define OBD_RQST_VEHICLE_INFO           0x09 /* Request vehicle information */
#define OBD_PERMANENT_DTC               0x0a /* Permanent Diagnostic Trouble Codes */

/* Basic Standardized Sensor/Status */

#define OBD_PID_SUPPORTED               0x00 /* PIDs supported 00-20 */
#define OBD_PID_STATUS                  0x01 /* Monitor status since DTCs cleared */
#define OBD_PID_STATUS_FREEZE_FRAME     0x02 /* DTC that caused required freeze frame data storage */
#define OBD_PID_FUEL_SYSTEM             0x03 /* Fuel system 1 and 2 status */
#define OBD_PID_ENGINE_LOAD             0x04 /* Calculated ENGINE LOAD Value */
#define OBD_PID_ENGINE_TEMPERATURE      0x05 /* Engine Coolant Temperature */
#define OBD_PID_SHORT_TERM_FUEL_TRIM13  0x06 /* Short Term Fuel Trim - Bank 1,3 */
#define OBD_PID_LONG_TERM_FUEL_TRIM13   0x07 /* Long Term Fuel Trim - Bank 1,3 */
#define OBD_PID_SHORT_TERM_FUEL_TRIM24  0x08 /* Short Term Fuel Trim - Bank 2,4 */
#define OBD_PID_LONG_TERM_FUEL_TRIM24   0x09 /* Long Term Fuel Trim - Bank 2,4 */
#define OBD_PID_FUEL_RAIL_PRESSURE      0x0a /* Fuel Rail Pressure (gauge) */
#define OBD_PID_MANIFOLD_ABS_PRESSURE   0x0b /* Intake Manifold Absolute Pressure (kPa) */
#define OBD_PID_RPM                     0x0c /* Engine RPM */
#define OBD_PID_SPEED                   0x0d /* Vehicle Speed Sensor */
#define OBD_PID_SPARK_ADVANCE           0x0e /* Ignition Timing Advance for #1 Cylinder */
#define OBD_PID_INTAKE_AIR_TEMPERATURE  0x0f /* Intake Air Temperature */
#define OBD_PID_MASS_AIR_FLOW           0x10 /* Air Flow Rate from Mass Air Flow Sensor */
#define OBD_PID_THROTTLE_POSITION       0x11 /* Absolute Throttle Position (0-100%) */
#define OBD_PID_AIR_STATUS              0x12 /* Commanded Secondary Air Status (Bit Encoded) */
#define OBD_PID_LOC_OXYGEN_SENSOR       0x13 /* Location of Oxygen Sensors (Bit Encoded) */
#define OBD_PID_OXYGEN_BANK1_SENSOR1    0x14 /* Bank 1 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK1_SENSOR2    0x15 /* Bank 1 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK1_SENSOR3    0x16 /* Bank 1 - Sensor 3 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK1_SENSOR4    0x17 /* Bank 1 - Sensor 4 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK2_SENSOR1    0x18 /* Bank 2 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK2_SENSOR2    0x19 /* Bank 2 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK2_SENSOR3    0x1a /* Bank 2 - Sensor 3 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_OXYGEN_BANK2_SENSOR4    0x1b /* Bank 2 - Sensor 4 Oxygen Sensor Output Voltage / Short Term Fuel Trim (V) */
#define OBD_PID_STANDARD_COMPLIANCE     0x1c /* OBD standards this vehicle conforms to */
#define OBD_PID_OXYGEN_SENSORS          0x1d /* Oxygen sensors present */
#define OBD_PID_AUXILIARY_INPUT_STATUS  0x1e /* Auxiliary input status */
#define OBD_PID_RUNTIME_ENGINE_START    0x1f /* Run time since engine start */

/* Extended Standardized Sensor_Status */

#define OBD_PID_SUPPORTED_EXT           0x20 /* PIDs supported 21-40 */
#define OBD_PID_DIST_TRAVELED_MIL       0x21 /* Distance traveled with malfunction indicator lamp (MIL) on */
#define OBD_PID_FUEL_RAIL_PRESS_VACUUM  0x22 /* Fuel Rail Pressure (relative to manifold vacuum) */
#define OBD_PID_FUEL_RAIL_PRESS_DIR_INJ 0x23 /* Fuel Rail Pressure (diesel, or gasoline direct inject) */
#define OBD_PID_O2S1_WR_LAMBDA_ERV      0x24 /* O2S1_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S2_WR_LAMBDA_ERV      0x25 /* O2S2_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S3_WR_LAMBDA_ERV      0x26 /* O2S3_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S4_WR_LAMBDA_ERV      0x27 /* O2S4_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S5_WR_LAMBDA_ERV      0x28 /* O2S5_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S6_WR_LAMBDA_ERV      0x29 /* O2S6_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S7_WR_LAMBDA_ERV      0x2a /* O2S7_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_O2S8_WR_LAMBDA_ERV      0x2b /* O2S8_WR_lambda(1): Equivalence Ratio Voltage */
#define OBD_PID_COMMANDED_EGR           0x2c /* Commanded EGR */
#define OBD_PID_EGR_ERROR               0x2d /* EGR Error */
#define OBD_PID_CMD_EVAPORAT_PURGE      0x2e /* Commanded evaporative purge */
#define OBD_PID_FUEL_LEVEL_INPUT        0x2f /* Fuel Level Input */
#define OBD_PID_WARMUP_CODES_CLEARED    0x30 /* Number of warm-ups since codes cleared */
#define OBD_PID_DIST_TRAV_CODES_CLEAR   0x31 /* Distance traveled since codes cleared */
#define OBD_PID_EVAP_SYS_VAPOR_PRESS    0x32 /* Evap. System Vapor Pressure */
#define OBD_PID_BAROMETRIC_PRESSURE     0x33 /* Barometric pressure */
#define OBD_PID_O2S1_WR_LAMBDA_ERC      0x34 /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S2_WR_LAMBDA_ERC      0x35 /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S3_WR_LAMBDA_ERC      0x36 /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S4_WR_LAMBDA_ERC      0x37 /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S5_WR_LAMBDA_ERC      0x38 /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S6_WR_LAMBDA_ERC      0x39 /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S7_WR_LAMBDA_ERC      0x3a /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_O2S8_WR_LAMBDA_ERC      0x3b /* O2S1_WR_lambda(1): Equivalence Ratio Current */
#define OBD_PID_CATAL_TEMP_BK1SS1       0x3c /* Catalyst Temperature Bank 1, Sensor 1 */
#define OBD_PID_CATAL_TEMP_BK2SS1       0x3d /* Catalyst Temperature Bank 2, Sensor 1 */
#define OBD_PID_CATAL_TEMP_BK1SS2       0x3e /* Catalyst Temperature Bank 1, Sensor 2 */
#define OBD_PID_CATAL_TEMP_BK2SS2       0x3f /* Catalyst Temperature Bank 2, Sensor 2 */

#endif /* __APPS_INCLUDE_CANUTILS_OBD_PID_H */
