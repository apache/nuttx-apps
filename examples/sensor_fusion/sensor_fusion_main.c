/****************************************************************************
 * apps/examples/sensor_fusion/sensor_fusion_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <nuttx/sensors/sensor.h>
#include "Fusion/Fusion.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define REG_LOW_MASK 0xFF00
#define REG_HIGH_MASK 0x00FF
#define MPU6050_FS_SEL 32.8f
#define MPU6050_AFS_SEL 4096.0f

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mpu6050_imu_msg
{
  int16_t acc_x;
  int16_t acc_y;
  int16_t acc_z;
  int16_t temp;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void read_mpu6050(int fd, struct sensor_accel *acc_data,
                  struct sensor_gyro *gyro_data);

/****************************************************************************
 * sensor_fusion_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int fd;
  int iterations = CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLES;
  float acq_period = CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLE_RATE / 1000.0f;

  /* Minimal required data for Fusion library. Use of magnetometer data
   * is optional and can improve performance.
   */

  struct sensor_accel imu_acc_data;
  struct sensor_gyro imu_gyro_data;
  FusionVector accelerometer;
  FusionVector gyroscope;
  FusionEuler euler;
  FusionAhrs ahrs;

  FusionAhrsInitialise(&ahrs);

  printf("Sensor Fusion example\n");
  printf("Sample Rate: %.2f Hz\n", 1.0 / acq_period);

  fd = open("/dev/imu0", O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open imu0\n");
      return EXIT_FAILURE;
    }

  for (int i = 0; i < iterations; i++)
    {
      read_mpu6050(fd, &imu_acc_data, &imu_gyro_data);

      accelerometer.axis.x = imu_acc_data.x;
      accelerometer.axis.y = imu_acc_data.y;
      accelerometer.axis.z = imu_acc_data.z;

      gyroscope.axis.x = imu_gyro_data.x;
      gyroscope.axis.y = imu_gyro_data.y;
      gyroscope.axis.z = imu_gyro_data.z;

      FusionAhrsUpdateNoMagnetometer(&ahrs,
                                     gyroscope,
                                     accelerometer,
                                     acq_period);
      euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));

      printf("Yaw: %.3f | Pitch: %.3f | Roll: %.3f\n",
             euler.angle.yaw, euler.angle.pitch, euler.angle.roll);
      usleep(CONFIG_EXAMPLES_SENSOR_FUSION_SAMPLE_RATE * 1000);
    }

  close(fd);

  return EXIT_SUCCESS;
}

void read_mpu6050(int fd,
                  struct sensor_accel *acc_data,
                  struct sensor_gyro *gyro_data)
{
  struct mpu6050_imu_msg raw_imu;
  int16_t raw_data[7];
  memset(&raw_imu, 0, sizeof(raw_imu));

  int ret = read(fd, &raw_data, sizeof(raw_data));
  if (ret != sizeof(raw_data))
    {
      printf("Failed to read accelerometer data\n");
    }
    else
    {
      raw_imu.acc_x = ((raw_data[0] & REG_HIGH_MASK) << 8) +
                        ((raw_data[0] & REG_LOW_MASK) >> 8);
      raw_imu.acc_y = ((raw_data[1] & REG_HIGH_MASK) << 8) +
                        ((raw_data[1] & REG_LOW_MASK) >> 8);
      raw_imu.acc_z = ((raw_data[2] & REG_HIGH_MASK) << 8) +
                        ((raw_data[2] & REG_LOW_MASK) >> 8);
      raw_imu.gyro_x = ((raw_data[4] & REG_HIGH_MASK) << 8) +
                        ((raw_data[4] & REG_LOW_MASK) >> 8);
      raw_imu.gyro_y = ((raw_data[5] & REG_HIGH_MASK) << 8) +
                        ((raw_data[5] & REG_LOW_MASK) >> 8);
      raw_imu.gyro_z = ((raw_data[6] & REG_HIGH_MASK) << 8) +
                        ((raw_data[6] & REG_LOW_MASK) >> 8);
    }

  acc_data->x = raw_imu.acc_x / MPU6050_AFS_SEL;
  acc_data->y = raw_imu.acc_y / MPU6050_AFS_SEL;
  acc_data->z = raw_imu.acc_z / MPU6050_AFS_SEL;

  /* Gyro data in º/s (degrees per second) */

  gyro_data->x = raw_imu.gyro_x / MPU6050_FS_SEL;
  gyro_data->y = raw_imu.gyro_y / MPU6050_FS_SEL;
  gyro_data->z = raw_imu.gyro_z / MPU6050_FS_SEL;
}
