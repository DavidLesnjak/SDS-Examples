/*
 * Copyright (c) 2024 Arm Limited. All rights reserved.
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

#include "recorder_sds.h"

#include <stdio.h>
#include <stdbool.h>

#include "RTE_Components.h"

#include "cmsis_os2.h"
#include "cmsis_vio.h"

#include "sds_rec.h"

// Configuration

// SDS Recorder data buffers
#ifndef REC_BUF_SIZE_MODEL_IN
#define REC_BUF_SIZE_MODEL_IN               8192U
#endif
#ifndef REC_BUF_SIZE_MODEL_OUT
#define REC_BUF_SIZE_MODEL_OUT              576U
#endif

// SDS Recorder IO thresholds
#ifndef REC_IO_THRESHOLD_MODEL_IN
#define REC_IO_THRESHOLD_MODEL_IN          (REC_BUF_SIZE_MODEL_IN-512U)
#endif
#ifndef REC_IO_THRESHOLD_MODEL_OUT
#define REC_IO_THRESHOLD_MODEL_OUT         (REC_BUF_SIZE_MODEL_OUT-64U)
#endif

#ifdef  RTE_SDS_IO_SOCKET
extern  int32_t socket_startup (void);
#endif

// Recorder error information
sds_error_t sds_error;

// Recorder active status
volatile uint8_t recActive = 0U;

// Recorder identifiers
sdsRecId_t recId_model_in  = NULL;
sdsRecId_t recId_model_out = NULL;

// Recorder buffers
static uint8_t recBuf_model_in [REC_BUF_SIZE_MODEL_IN];
static uint8_t recBuf_model_out[REC_BUF_SIZE_MODEL_OUT];

// Recorder event callback
static void recorder_event_callback (sdsRecId_t id, uint32_t event) {
  if ((event & SDS_REC_EVENT_IO_ERROR) != 0U) {
    sds_assert(false);
  }
}

// Recording control, toggle start/stop via USER Button press.
// Toggle LED0 every 1 second to see this control is alive.
// Turn on LED1 when recording is started, turn it off when recording is stopped.
void rec_control (void) {
  uint8_t btn_val;
  uint8_t btn_prev = 0U;
  uint8_t led0_cnt = 0U;
  uint8_t led0_val = 0U;
  int32_t status;

#ifdef RTE_SDS_IO_SOCKET
  // Initialize socket interface
  status = socket_startup();
  sds_assert(status == 0);
#endif

  // Initialize SDS recorder
  status = sdsRecInit(recorder_event_callback);
  sds_assert(status == SDS_REC_OK);

  for (;;) {
    // Toggle LED0 every 1 second
    if (led0_cnt++ >= 10U) {
      led0_cnt  = 0U;
      led0_val ^= 1U;
      vioSetSignal(vioLED0, led0_val);
    }

    // Monitor user button
    btn_val = vioGetSignal(vioBUTTON0);
    if (btn_val != btn_prev) {
      // If push-button state has changed
      btn_prev = btn_val;
      if (btn_val == vioBUTTON0) {      // If push-button is pressed
        if (recActive == 0U) {
          recActive = 1U;

          // Start recording of Model Input data
          recId_model_in  = sdsRecOpen("Model_Input",
                                        recBuf_model_in,
                                        sizeof(recBuf_model_in),
                                        REC_IO_THRESHOLD_MODEL_IN);
          sds_assert(recId_model_in != NULL);

          // Start recording of Model Output data
          recId_model_out = sdsRecOpen("Model_Output",
                                        recBuf_model_out,
                                        sizeof(recBuf_model_out),
                                        REC_IO_THRESHOLD_MODEL_OUT);
          sds_assert(recId_model_out != NULL);

          // If recording was started turn LED1 on
          vioSetSignal(vioLED1, vioLEDon);
        } else {
          recActive = 0U;

          // Stop recording of Model Input data
          status = sdsRecClose(recId_model_in);
          sds_assert(status == SDS_REC_OK);

          // Stop recording of Model Output data
          status = sdsRecClose(recId_model_out);
          sds_assert(status == SDS_REC_OK);

          // If recording was stopped turn LED1 off
          vioSetSignal(vioLED1, vioLEDoff);
        }
      }
    }
    osDelay(100U);                      // Delay for button debouncing
  }
}
