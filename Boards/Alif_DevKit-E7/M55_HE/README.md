# Board: AlifSemiconductor DevKit

## Board Layer for M55 High Efficiency Core

Device: AE722F80F55D5LS:M55_HE

This setup is configured using **Conductor Tool**, an interactive tool provided by Alif Semiconductor for device configuration.
Refer to ["Alif Conductor Tool Manual"](https://conductor.alifsemi.com/Alif_HTML_DCT_User_Help/Content/Help%20Manual.htm) for additional information.

### System Configuration

| System Component        | Setting
|:------------------------|:----------------------------------------
| Heap                    | 96 kB (configured in M55_HE linker file)
| Stack (MSP)             | 8 kB (configured in M55_HE linker file)

### STDIO mapping

**STDIO** is routed to Virtual COM port via **UART4** peripheral

### Virtual Streaming Interfaces

| Interface | Peripheral | Board connector/component | Connection
|:----------|:-----------|:--------------------------|:-----------
| Audio     | SAI3       | I2S Microphone (U13, U14) | AVH_AUDIO

### CMSIS-Driver mapping

| CMSIS-Driver        | Peripheral | Board connector/component     | Connection
|:--------------------|:-----------|:------------------------------|:------------------------------
| Driver_ETH_MAC0     | ETH        | Ethernet RJ45 connector (J28) | CMSIS_ETH
| Driver_USART4       | UART4      | PRG USB connector (J3)        | STDIN, STDOUT, STDERR
| CMSIS-Driver VIO    | GPIO       | RGB LED (D10), Joystick (SW1) | CMSIS_VIO

### CMSIS-Driver Virtual I/O mapping

| CMSIS-Driver VIO  | Board component
|:------------------|:----------------------------
| vioBUTTON0        | Joystick Select Button (SW1)
| vioLED0           | RGB LED Red (D10)
| vioLED1           | RGB LED Green (D10)
| vioLED2           | RGB LED Blue (D10)
