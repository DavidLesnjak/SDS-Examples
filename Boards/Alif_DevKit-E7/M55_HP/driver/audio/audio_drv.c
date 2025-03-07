/*---------------------------------------------------------------------------
 * Copyright (c) 2025 Arm Limited (or its affiliates).
 * All rights reserved.
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
 *---------------------------------------------------------------------------*/

 #include <stddef.h>
#include <string.h>
#include "audio_drv.h"
#include "Driver_SAI.h"

#ifdef _RTE_
#include "RTE_Components.h"
#endif
#include CMSIS_device_header
#include CMSIS_target_header

/* Configuration */
#define AUDIO_CFG_SAI_MODE      ARM_SAI_MODE_MASTER
#define AUDIO_CFG_SAI_SYNC      ARM_SAI_ASYNCHRONOUS
#define AUDIO_CFG_SAI_PROTOCOL  ARM_SAI_PROTOCOL_I2S

#define AUDIO_CFG_SAI_INSTANCE  CMSIS_DRIVER_SAI

/* Definitions */
#define AUDIO_FLAGS_INIT      (1U << 0)

typedef struct audio_buf_s {
  void    *data;              /* Data buffer pointer     */
  uint32_t block_num;         /* Number of data blocks   */
  uint32_t block_size;        /* Data block size         */
} AudioBuf_t;

typedef struct audio_cb_s {
  ARM_DRIVER_SAI *drv;        /* SAI driver instance     */
  AudioDrv_Event_t  callback; /* Audio driver callback   */
  AudioDrv_Status_t status;   /* Audio driver status     */
  AudioBuf_t tx_buf;          /* Transmit buffer info    */
  AudioBuf_t rx_buf;          /* Receive buffer info     */
  uint32_t   rx_cnt;          /* Receiver block count    */
  uint32_t   tx_cnt;          /* Transmitter block count */
  uint8_t    flags;           /* Audio driver flags      */
} AudioCb_t;

/* Driver Control Block */
static AudioCb_t AudioCb;

/**
  \brief Callback routine from the SAI driver.

  \param[in]  event  Event for which the callback has been called.
*/
static void Driver_SAI_Callback (uint32_t event) {
  uint32_t buf_block;
  uint32_t buf_index;

  if (event & ARM_SAI_EVENT_RECEIVE_COMPLETE) {
    /* Increment receive block count */
    AudioCb.rx_cnt++;

    /* Determine current receive buffer block */
    buf_block = AudioCb.rx_cnt % AudioCb.rx_buf.block_num;
    /* Determine current buffer index */
    buf_index = buf_block * AudioCb.rx_buf.block_size;

    /* Trigger new receive */
    AudioCb.drv->Receive(&AudioCb.rx_buf.data[buf_index], AudioCb.rx_buf.block_size/2);

    /* Call application callback function */
    if (AudioCb.callback != NULL) {
      AudioCb.callback(AUDIO_DRV_EVENT_RX_DATA);
    }
  }
  if (event & ARM_SAI_EVENT_SEND_COMPLETE) {
    /* Increment transmitter block count */
    AudioCb.tx_cnt++;

    /* Determine current transmit buffer block */
    buf_block = AudioCb.tx_cnt % AudioCb.tx_buf.block_num;

    /* Determine current buffer index */
    buf_index = AudioCb.tx_cnt * AudioCb.tx_buf.block_size;

    /* Trigger new transmit */
    AudioCb.drv->Send(&AudioCb.tx_buf.data[buf_index], AudioCb.tx_buf.block_size/2);

    /* Call application callback function */
    if (AudioCb.callback != NULL) {
      AudioCb.callback(AUDIO_DRV_EVENT_TX_DATA);
    }
  }
  if (event & ARM_SAI_EVENT_TX_UNDERFLOW) {
  }
  if (event & ARM_SAI_EVENT_RX_OVERFLOW) {
  }
}


/* Initialize Audio Interface */
int32_t AudioDrv_Initialize (AudioDrv_Event_t cb_event) {
  int32_t status;

  AudioCb.callback = cb_event;
  AudioCb.flags    = 0U;

  /* Initialize SAI */
  AudioCb.drv = &ARM_Driver_SAI_(AUDIO_CFG_SAI_INSTANCE);

  if (AudioCb.drv->Initialize(Driver_SAI_Callback) != ARM_DRIVER_OK) {
    status = AUDIO_DRV_ERROR;
  }
  else if (AudioCb.drv->PowerControl(ARM_POWER_FULL) != ARM_DRIVER_OK) {
    status = AUDIO_DRV_ERROR;
  }
  else {
    /* Driver is initialized */
    AudioCb.flags |= AUDIO_FLAGS_INIT;

    status = AUDIO_DRV_OK;
  }

  return (status);
}

/* De-initialize Audio Interface */
int32_t AudioDrv_Uninitialize (void) {

  /* Stop SAI transmitter */
  AudioCb.drv->Control(ARM_SAI_CONTROL_TX, 0U, 0U);

  if (AudioCb.status.tx_active == 1U) {
    /* Abort SAI send transfer */
    AudioCb.drv->Control(ARM_SAI_ABORT_SEND, 0U, 0U);
  }

  /* Stop SAI receiver */
  AudioCb.drv->Control(ARM_SAI_CONTROL_RX, 0U, 0U);

  if (AudioCb.status.rx_active == 1U) {
    /* Abort SAI receive transfer */
    AudioCb.drv->Control(ARM_SAI_ABORT_RECEIVE, 0U, 0U);
  }

  /* Power down and uninitialize SAI */
  AudioCb.drv->PowerControl(ARM_POWER_OFF);
  AudioCb.drv->Uninitialize();

  /* Clear audio control block structure */
  memset(&AudioCb, 0, sizeof(AudioCb));

  return AUDIO_DRV_OK;
}

/* Configure Audio Interface */
int32_t AudioDrv_Configure (uint32_t interface, uint32_t channels, uint32_t sample_bits, uint32_t sample_rate) {
  int32_t status;

  if ((interface != AUDIO_DRV_INTERFACE_TX) && (interface != AUDIO_DRV_INTERFACE_RX)) {
    status = AUDIO_DRV_ERROR_PARAMETER;
  }
  else if ((channels    <  1U) || (channels    > 32U) ||
           (sample_bits <  8U) || (sample_bits > 32U) ||
           (sample_rate == 0U)) {
    status = AUDIO_DRV_ERROR_PARAMETER;
  }
  else if ((AudioCb.flags & AUDIO_FLAGS_INIT) == 0U) {
    /* Not initialized */
    status = AUDIO_DRV_ERROR;
  }
  else {
    status = AUDIO_DRV_OK;

    if (interface == AUDIO_DRV_INTERFACE_RX) {
      if (AudioCb.status.rx_active == 0U) {
        /* Configure receive channel */
        status = AudioCb.drv->Control(ARM_SAI_CONFIGURE_RX |
                                      AUDIO_CFG_SAI_MODE | AUDIO_CFG_SAI_SYNC | AUDIO_CFG_SAI_PROTOCOL |
                                      ARM_SAI_DATA_SIZE(sample_bits),
                                      sample_bits * 2,
                                      sample_rate);
      }
      else {
        status = AUDIO_DRV_ERROR_BUSY;
      }
    }
    else /* if (interface == AUDIO_DRV_INTERFACE_TX) */ {
      if (AudioCb.status.tx_active == 0U) {
        /* Configure transmit channel */
        status = AudioCb.drv->Control(ARM_SAI_CONFIGURE_TX |
                                      AUDIO_CFG_SAI_MODE | AUDIO_CFG_SAI_SYNC | AUDIO_CFG_SAI_PROTOCOL |
                                      ARM_SAI_DATA_SIZE(sample_bits),
                                      sample_bits * 2,
                                      sample_rate);
      }
      else {
        status = AUDIO_DRV_ERROR_BUSY;
      }
    }
  }

  return (status);
}

/* Set Audio Interface buffer */
int32_t AudioDrv_SetBuf (uint32_t interface, void *buf, uint32_t block_num, uint32_t block_size) {
  int32_t status;
  AudioBuf_t *p;

  if ((interface != AUDIO_DRV_INTERFACE_TX) && (interface != AUDIO_DRV_INTERFACE_RX)) {
    status = AUDIO_DRV_ERROR_PARAMETER;
  }
  else if (buf == NULL) {
    status = AUDIO_DRV_ERROR_PARAMETER;
  }
  else if ((block_num == 0U) || (block_size == 0U)) {
    status = AUDIO_DRV_ERROR_PARAMETER;
  }
  else {
    status = AUDIO_DRV_OK;

    p = NULL;

    if (interface == AUDIO_DRV_INTERFACE_RX) {
      if (AudioCb.status.rx_active == 0U) {
        /* Set receive buffer */
        p = &AudioCb.rx_buf;
      }
      else {
        status = AUDIO_DRV_ERROR_BUSY;
      }
    }
    else /* if (interface == AUDIO_DRV_INTERFACE_TX) */ {
      if (AudioCb.status.tx_active == 0U) {
        /* Set transmit buffer */
        p = &AudioCb.tx_buf;
      }
      else {
        status = AUDIO_DRV_ERROR_BUSY;
      }
    }

    if (p != NULL) {
      p->data       = buf;
      p->block_num  = block_num;
      p->block_size = block_size;
    }
  }

  return (status);
}

/* Control Audio Interface */
int32_t AudioDrv_Control (uint32_t control) {
  int32_t status;

  if ((AudioCb.flags & AUDIO_FLAGS_INIT) == 0U) {
    /* Not initialized */
    return AUDIO_DRV_ERROR;
  }

  status = AUDIO_DRV_OK;

  if ((control & AUDIO_DRV_CONTROL_TX_ENABLE) != 0U) {
    if (AudioCb.tx_buf.data == NULL) {
      /* Transmit buffer is not set */
      status = AUDIO_DRV_ERROR;
    }
    else if (AudioCb.status.tx_active == 1U) {
      /* Transmit in progress */
      status = AUDIO_DRV_ERROR_BUSY;
    }
    else {
      /* Start transmit operation */
      AudioCb.status.tx_active = 1U;
      AudioCb.tx_cnt           = 0U;

      /* Register SAI send operation */
      if (AudioCb.drv->Send(AudioCb.tx_buf.data, AudioCb.tx_buf.block_size/2) != ARM_DRIVER_OK) {
        status = AUDIO_DRV_ERROR;
      }
      else {
        /* Enable SAI transmitter */
        if (AudioCb.drv->Control(ARM_SAI_CONTROL_TX, 1U, 0U) != ARM_DRIVER_OK) {
          status = AUDIO_DRV_ERROR;
        } else {
          status = AUDIO_DRV_OK;
        }
      }
    }
  }
  else if ((control & AUDIO_DRV_CONTROL_TX_DISABLE) != 0U) {
    /* Disable SAI transmitter */
    if (AudioCb.drv->Control(ARM_SAI_CONTROL_TX, 0U, 0U) != ARM_DRIVER_OK) {
      status = AUDIO_DRV_ERROR;
    }

    AudioCb.status.tx_active = 0U;
  }
  else if ((control & AUDIO_DRV_CONTROL_RX_ENABLE) != 0U) {
    if (AudioCb.rx_buf.data == NULL) {
      /* Receive buffer is not set */
      status = AUDIO_DRV_ERROR;
    }
    else if (AudioCb.status.rx_active == 1U) {
      /* Receive in progress */
      status = AUDIO_DRV_ERROR_BUSY;
    }
    else {
      /* Start receive operation */
      AudioCb.status.rx_active = 1U;
      AudioCb.rx_cnt           = 0U;

      /* Register SAI receive operation */
      if (AudioCb.drv->Receive(AudioCb.rx_buf.data, AudioCb.rx_buf.block_size/2) != ARM_DRIVER_OK) {
        status = AUDIO_DRV_ERROR;
      }
      else {
        /* Enable SAI receiver */
        if (AudioCb.drv->Control(ARM_SAI_CONTROL_RX, 1U, 0U) != ARM_DRIVER_OK) {
          status = AUDIO_DRV_ERROR;
        } else {
          status = AUDIO_DRV_OK;
        }
      }
    }
  }
  else if ((control & AUDIO_DRV_CONTROL_RX_DISABLE) != 0U) {
    /* Disable SAI receiver */
    if (AudioCb.drv->Control(ARM_SAI_CONTROL_RX, 0U, 0U) != ARM_DRIVER_OK) {
      status = AUDIO_DRV_ERROR;
    }

    AudioCb.status.rx_active = 0U;
  }
  else {
    status = AUDIO_DRV_ERROR_UNSUPPORTED;
  }

  return (status);
}

/* Get transmitted block count */
uint32_t AudioDrv_GetTxCount (void) {
  return (AudioCb.tx_cnt);
}

/* Get received block count */
uint32_t AudioDrv_GetRxCount (void) {
  return (AudioCb.rx_cnt);
}

/* Get Audio Interface status */
AudioDrv_Status_t AudioDrv_GetStatus (void) {
  return (AudioCb.status);
}
