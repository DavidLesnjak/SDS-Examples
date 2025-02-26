# About this application

This is an standalone application using [Edge Impulse](https://edgeimpulse.com/) Machine Learning and
[SDS Recorder](https://github.com/ARM-software/SDS-Framework) for recording ML model input and output data.

The example has trained model for continuous motion recognition using an tutorial from the Edge Impulse
(Tutorial: Continuous motion recognition).

Required Edge Impulse packs are located in the `./Packs` sub-folder and need to be installed.
 
This example is pre-configured for STMicroelectronics [STM32H735G-DK](https://www.st.com/en/evaluation-tools/stm32h735g-dk.html) board.

It uses Ethernet interface for recording the data on the host machine.

Model input data:
- const data containing 1 window of 2 seconds worth of data (375 float numbers) stored in an `features` array in the 
  `features.h` file

Model output data:
- results of inference executed every 2 seconds, containing probabilities for 4 possible classification groups

Examples of 20 second of captured data are available in the `./SDS Recordings` sub-folder, containing also
`.sds.yml` metadata files.

The debug output of this example running looks like below:

```
Edge Impulse standalone inferencing
Inferencing settings:
.Classifier interval: 16.00 ms.
.Input frame size: 375
.Raw sample count: 125 samples.
.Raw samples per frame: 3 samples.
.Number of output classes: 4
Predictions (DSP: 1.000000 ms., Classification: 0 ms., Anomaly: 0ms.): 
#Classification results:
    idle: 0.000000
    snake: 0.000000
    updown: 0.003906
    wave: 0.996094
Anomaly prediction: -0.183448
```

> Note: CMSIS-NN in v4.1.0 did not include `arm_s8_to_s16_unordered_with_offset.c` file necessary 
>       for this example. It must be added manually for this example to build.