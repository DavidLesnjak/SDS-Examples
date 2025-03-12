/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Sensor Driver
#include <string.h>

#include "bmi323_icm42670.h"

#include "sensor_config.h"
#include "sensor_drv.h"

#ifdef SENSOR0_NAME
static sensorConfig_t sensorConfig0 = {
  SENSOR0_NAME,
  SENSOR0_DMA_MODE,
  SENSOR0_SAMPLE_SIZE,
  #if SENSOR0_DMA_MODE == 0U
  { {
    SENSOR0_SAMPLE_INTERVAL,
    SENSOR0_FIFO_SIZE,
    SENSOR0_DATA_THRESHOLD
  } }
  #else
  { {
    SENSOR0_BLOCK_INTERVAL,
    SENSOR0_BLOCK_SIZE,
    SENSOR0_BLOCK_NUM
  } }
  #endif
};
#endif

#ifdef SENSOR1_NAME
static sensorConfig_t sensorConfig1 = {
  SENSOR1_NAME,
  SENSOR1_DMA_MODE,
  SENSOR1_SAMPLE_SIZE,
  #if SENSOR1_DMA_MODE == 0U
  { {
    SENSOR1_SAMPLE_INTERVAL,
    SENSOR1_FIFO_SIZE,
    SENSOR1_DATA_THRESHOLD
  } }
  #else
  { {
    SENSOR1_BLOCK_INTERVAL,
    SENSOR1_BLOCK_SIZE,
    SENSOR1_BLOCK_NUM
  } }
  #endif
};
#endif

typedef struct {
  sensorConfig_t *config;
  sensorStatus_t  status;
} sensor_t;

static sensor_t Sensors[2] = {
#ifdef SENSOR0_NAME
  { &sensorConfig0, {0U, 0U, 0U} },
#endif
#ifdef SENSOR1_NAME
  { &sensorConfig1,  {0U, 0U, 0U} },
#endif
};

// Get sensor identifier
sensorId_t sensorGetId (const char *name) {
  sensor_t *sensor = NULL;
  uint32_t n;

  if (name != NULL) {
    for (n = 0U; n < (sizeof(Sensors) / sizeof(sensor_t)); n++) {
      if (strcmp(name, Sensors[n].config->name) == 0) {
        sensor = &Sensors[n];
        break;
      }
    }
  }
  return sensor;
}

// Get sensor configuration
sensorConfig_t *sensorGetConfig (sensorId_t id) {
  sensor_t *sensor = id;
  sensorConfig_t *cfg = NULL;

  if (sensor != NULL) {
    cfg = sensor->config;
  }
  return cfg;
}

// Register sensor events
int32_t sensorRegisterEvents (sensorId_t id, sensorEvent_t event_cb, uint32_t event_mask) {
  // Events are not suppported
  return SENSOR_ERROR;
}

// Enable sensor
int32_t sensorEnable (sensorId_t id) {
  sensor_t *sensor = id;
  int32_t ret = SENSOR_ERROR;

  if (sensor != NULL) {
    if ((Sensors[0].status.active == 0U) || (Sensors[1].status.active == 0U)) {
      if ((bmi323_icm42670_init() & 0x01) == 0x01) {
        Sensors[0].status.active = 1U;
        Sensors[1].status.active = 1U;
        ret = SENSOR_OK;
      }
    }
  }
  return ret;
}

// Disable sensor
int32_t sensorDisable (sensorId_t id) {
  return SENSOR_ERROR;          // Disable not supported
}

// Get sensor status
sensorStatus_t sensorGetStatus (sensorId_t id) {
  sensor_t *sensor = id;
  sensorStatus_t stat = {0U, 0U, 0U};

  if (sensor != NULL) {
    stat.active   = sensor->status.active;
    stat.overflow = 0U;         // Overflow not supported
  }
  return stat;
}

// Read samples from sensor
uint32_t sensorReadSamples (sensorId_t id, uint32_t num_samples, void *buf, uint32_t buf_size) {
  sensor_t *sensor = id;
  uint32_t num = 0U;
  xyz_accel_gyro_imu_s sample;

  if ((sensor != NULL) && (num_samples != 0U) && (buf != NULL) &&
      (buf_size >= (num_samples * sensor->config->sample_size))) {

    if (sensor == &Sensors[0]) {
      if (getBMI323Accel(&sample) == 0) {
        memcpy(buf, &(sample.acc_x), sensor->config->sample_size);
        num = 1U;
      }
    } else if (sensor == &Sensors[1]) {
      if (getBMI323Gyro(&sample) == 0) {
        memcpy(buf, &(sample.gyro_x), sensor->config->sample_size);
        num = 1U;
      }
    }
  }
  return num;
}

// Get block data
void *sensorGetBlockData (sensorId_t id) {
  return NULL;
}

