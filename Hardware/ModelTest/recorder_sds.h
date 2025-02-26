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

#ifndef RECORDER_SDS_H_
#define RECORDER_SDS_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C"
{
#endif

#include "sds_rec.h"

// Assert macro
#define sds_assert(cond)        if (((cond) == false) && (sds_error.occurred == 0U)) { sds_error.occurred = 1U; sds_error.file = __FILE__; sds_error.line = __LINE__; }

// Recorder error information structure
typedef struct {
  uint8_t     occurred;
  uint8_t     reported;
  const char *file;
  uint32_t    line;
} sds_error_t;

// Recorder error information
extern sds_error_t sds_error;

// Recorder active status
extern volatile uint8_t recActive;

// Recorder identifiers
extern sdsRecId_t recId_model_in;
extern sdsRecId_t recId_model_out;

// Recording control
extern void rec_control (void);

#ifdef  __cplusplus
}
#endif

#endif