/*
 * Copyright (c) 2024 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include "ei_run_classifier.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_variables.h"
#include "features.h"
#include "cmsis_os2.h"
#include "rec_management.h"

// Timestamp of read of input data for inference
uint32_t timestamp;

/**
 * @brief      Get raw features from the static features array
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr)
{
    memcpy(out_ptr, features + offset, length * sizeof(float));

    // If recording is active then record model input data
    if (recActive != 0U) {
      timestamp = osKernelGetTickCount();
      uint32_t num = sdsRecWrite(recIdModelInput, timestamp, features + offset, length * sizeof(float));
      REC_ASSERT(num == (length * sizeof(float)));
    }

    return 0;
}

/**
 * @brief      Get the raw features from the sensor and run the classifier
 */
extern "C" int ei_main(void)
{
    ei_impulse_result_t result = {nullptr};

    ei_printf("Edge Impulse standalone inferencing\n");

    if (FEATURES_SIZE != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'features' array is not correct. Expected %d items, but had %u\n",
                EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, FEATURES_SIZE);
        return 1;
    }

    // summary of inferencing settings (from model_metadata.h)
    printf("Inferencing settings:\n");
    printf("\tClassifier interval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    printf("\tInput frame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    printf("\tRaw sample count: %d samples.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    printf("\tRaw samples per frame: %d samples.\n", EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
    printf("\tNumber of output classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
        
    // the features are stored into flash, and we don't want to load everything into RAM
    signal_t features_signal;
    features_signal.total_length = sizeof(features) / sizeof(features[0]);
    features_signal.get_data = &raw_feature_get_data;

    while (1) {
        // Input: 375 float numbers (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

        if (res != 0) {
            ei_printf("ERR: Failed to run classifier\n");
            return 1;
        }

        display_results(&ei_default_impulse, &result);

        // Output, for example:
        // idle: 0.996094
        // snake: 0.000000
        // updown: 0.000000
        // wave: 0.000000

        // If recording is active then record model output data
        if (recActive != 0U) {
          // Prepare model output data for recording
          float model_out_results[EI_CLASSIFIER_NN_OUTPUT_COUNT];

          for (uint8_t i = 0U; i < EI_CLASSIFIER_NN_OUTPUT_COUNT; i++) {
            model_out_results[i] = result.classification[i].value;
          }

          // Record model output data
          uint32_t num = sdsRecWrite(recIdModelOutput, timestamp, model_out_results, EI_CLASSIFIER_NN_OUTPUT_COUNT * sizeof(float));
          REC_ASSERT(num == (EI_CLASSIFIER_NN_OUTPUT_COUNT * sizeof(float)));
        }

        ei_sleep(2000);
    }

    return 0;
}
