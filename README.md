# Z906 Remote

![Z906 banner](/../docs/images/logitech_z906.webp?raw=true "Z906 banner")

|            | develop                                                                                                        | master                                                                                                        |
| ---------- | -------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| VueJS App  | ![workflow status](https://github.com/nforay/Z906Remote/actions/workflows/nodejs.yml/badge.svg?branch=develop) | ![workflow status](https://github.com/nforay/Z906Remote/actions/workflows/nodejs.yml/badge.svg?branch=master) |
| PlatformIO | ![workflow status](https://github.com/nforay/Z906Remote/actions/workflows/pio.yml/badge.svg?branch=develop)    | ![workflow status](https://github.com/nforay/Z906Remote/actions/workflows/pio.yml/badge.svg?branch=master)    |

- [Z906 Remote](#z906-remote)
	- [Overview](#overview)
	- [Recommended Hardware](#recommended-hardware)
	- [Usage](#usage)
		- [Get the code](#get-the-code)
		- [Edit Environment variables](#edit-environment-variables)
		- [Build](#build)
		- [Flash](#flash)
	- [Wiring](#wiring)
		- [Pinout](#pinout)
		- [Serial Communication](#serial-communication)
	- [WEB API](#web-api)
			- [Example Usage](#example-usage)
	- [TODO](#todo)

## Overview

This project enhances the **Logitech Z906**, a 5.1 THX-certified surround sound system, with WiFi control capabilities. It allows for remote management of the amplifier built into the subwoofer using an external console attached via a [DE-15 Connector](https://en.wikipedia.org/wiki/D-subminiature#DE-15_connectors).
Credits to [zarpli](https://github.com/zarpli/Logitech-Z906), [LewisSmallwood](https://github.com/LewisSmallwood/IoT-Logitech-Z906), and [nomis](https://github.com/nomis/logitech-z906) for their foundational work.

## Recommended Hardware

- [D1 Mini (ESP8266)](https://www.az-delivery.de/products/d1-mini)
- [DE-15 Female Socket to Terminal Block Breakout](https://www.adafruit.com/product/3124)

## Usage

### Get the code

```shell
git clone https://github.com/nforay/Z906Remote.git
cd Z906Remote
```

### Edit Environment variables

Duplicate the template files

```shell
cp back/include/environment.h.tpl back/include/environment.h
cp back/params.ini.tpl back/params.ini
```

Edit `back/include/environment.h` with your WiFi network credentials and set a password for future OTA Updates.

Modify `back/params.ini` to set the OTA Update password.

### Build

The simplest method is utilizing [PlatformIO IDE for VSCode](https://docs.platformio.org/page/ide/vscode.html#quick-start). Click the **Build** icon in the [PlatformIO Toolbar](https://docs.platformio.org/en/latest/integration/ide/vscode.html#platformio-toolbar).

### Flash

**First flash must be done using serial.**

If you're using a **D1 Mini** or simiar with a CH340 USB to Serial Chip install the [CH340 Driver](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all#windows-710).

Plug the D1 Mini, set the COM Port and click the **Upload** button in the [PlatformIO Toolbar](https://docs.platformio.org/en/latest/integration/ide/vscode.html#platformio-toolbar).

## Wiring

### Pinout

| Pin | Console                   | Amplifier                          |
| --- | ------------------------- | ---------------------------------- |
| 1   |                           | Headphones Right                   |
| 2   |                           | Headphones Left                    |
| 3   | Audio Ground              | Audio Ground                       |
| 4   | Aux Right                 |                                    |
| 5   | Aux Left                  |                                    |
| 6   | Ground                    | Ground                             |
| 7   | Unknown (not required)    | Unknown (not required)             |
| 8   | Pull-down (0V) resistor   | Output 3.3V after comms start/stop |
| 9   | Output 3.3V when powered  | Unused                             |
| 10  | Unknown (not required)    | Unused                             |
| 11  | Power supply              | 3.3V                               |
| 12  | Rx                        | Tx                                 |
| 13  | Tx                        | Rx                                 |
| 14  | Unused                    | Unused                             |
| 15  | Output 0V if comms active | Pull-up (3.3V) resistor            |

The following table shows the minimal pinout requirements. For Aux Input usage, wire a TRS Jack plug from pins 3 to 5.

| Pin | Description  | Pin | Description    |
| --- | ------------ | --- | -------------- |
| 3   | GND          | 6   | GND            |
| 11  | 3.3V @ 250mA | 12  | TX             |
| 13  | RX           | 15  | Console Enable |

![Circuit diagram](/../docs/images/circuit-diagram.png?raw=true "Circuit diagram")

### Serial Communication

The Rx/Tx pins are 3.3V TTL serial at a baud rate of 57600 bps with 8 bits per byte, odd parity and 1 stop bit. All communication is initiated by the console.

## WEB API

The project makes the following HTTP endpoints available to control all the functions of the Logitech Z906 over the API.

When making a web request to any of these endpoints, the microcontroller will run the desired action, interfacing with the Logitech Z906.

| Endpoint               | Parameters   | Serial Command    | Description                                    |
| ---------------------- | ------------ | ----------------- | ---------------------------------------------- |
| /input                 | -            | -                 | Get the currently selected input               |
| /input/0               | -            | SELECT_INPUT_1    | Swap to the TRS 5.1 input                      |
| /input/1               | -            | SELECT_INPUT_2    | Swap to the RCA 2.0 input                      |
| /input/2               | -            | SELECT_INPUT_3    | Swap to the Optical 1 input                    |
| /input/3               | -            | SELECT_INPUT_4    | Swap to the Optical 2 input                    |
| /input/4               | -            | SELECT_INPUT_5    | Swap to the Coaxial input                      |
| /input/5               | -            | SELECT_INPUT_AUX  | Swap to the TRS 2.0 console input              |
| /input/disable         | -            | BLOCK_INPUTS      | Disable signal input                           |
| /input/enable          | -            | NO_BLOCK_INPUTS   | Enable signal input                            |
|                        |
| /volume/main           | -            | MAIN_LEVEL        | Return the current Main Level                  |
| /volume/main/set       | value: 0-255 | MAIN_LEVEL        | Set the Main Level to the parameter value      |
| /volume/main/up        | -            | LEVEL_MAIN_UP     | Increase Main Level by one unit                |
| /volume/main/down      | -            | LEVEL_MAIN_DOWN   | Decrease Main Level by one unit                |
| /volume/subwoofer      | -            | SUB_LEVEL         | Return the current Subwoofer Level             |
| /volume/subwoofer/set  | value: 0-255 | SUB_LEVEL         | Set the Subwoofer Level to the parameter value |
| /volume/subwoofer/up   | -            | LEVEL_SUB_UP      | Increase Subwoofer Level by one unit           |
| /volume/subwoofer/down | -            | LEVEL_SUB_DOWN    | Decrease Subwoofer Level by one unit           |
| /volume/center         | -            | CENTER_LEVEL      | Return the current Center Level                |
| /volume/center/set     | value: 0-255 | CENTER_LEVEL      | Set the Center Level to the parameter value    |
| /volume/center/up      | -            | LEVEL_CENTER_UP   | Increase Center Level by one unit              |
| /volume/center/down    | -            | LEVEL_CENTER_DOWN | Decrease Subwoofer Level by one unit           |
| /volume/rear           | -            | REAR_LEVEL        | Return the current Rear Level                  |
| /volume/rear/set       | value: 0-255 | REAR_LEVEL        | Set the Rear Level to the parameter value      |
| /volume/rear/up        | -            | LEVEL_REAR_UP     | Increase Rear Level by one unit                |
| /volume/rear/down      | -            | LEVEL_REAR_DOWN   | Decrease Rear Level by one unit                |
|                        |
| /input/decode          | -            | -                 | Get the Decode Mode state                      |
| /input/decode/on       | -            | SELECT_EFFECT_51  | Enable Decode Mode                             |
| /input/decode/off      | -            | DISABLE_EFFECT_51 | Disable Decode Mode                            |
|                        |
| /input/effect/0        | -            | SELECT_EFFECT_3D  | Enable 3D Effect in current input              |
| /input/effect/1        | -            | SELECT_EFFECT_21  | Enable 2.1 Effect in current input             |
| /input/effect/2        | -            | SELECT_EFFECT_41  | Enable 4.1 Effect in current input             |
| /input/effect/3        | -            | SELECT_EFFECT_NO  | Disable all Effects in current input           |
|                        |
| /save                  | -            | EEPROM_SAVE       | Save current settings to EEPROM*               |
|                        |
| /mute                  | -            | -                 | Get muted status                               |
| /mute/on               | -            | MUTE_ON           | Enable Mute                                    |
| /mute/off              | -            | MUTE_OFF          | Disable Mute                                   |
|                        |
| /status                | -            | -                 | Get system status from buffer                  |
| /temperature           | -            | GET_TEMP          | Gets the system temperature                    |
| /version               | -            | VERSION           | Gets the system firmware version               |
| /power                 | -            | STATUS_STBY       | Get the current standby status                 |
| /power/on              | -            | PWM_ON            | Turn the system on                             |
| /power/off             | -            | PWM_OFF           | Turn the system off                            |

*Please note, use the **EEPROM_SAVE** function with caution. Each EEPROM has a limited number of write cycles (~100,000) per address. If you write excessively to the EEPROM, you will reduce the lifespan.

#### Example Usage

The API can be called through any browser. For example:

`GET /status` should respond like this:
![HTTP Request](/../docs/images/request.png?raw=true "HTTP Request")

## TODO
- Integrate MQTT functionality
- Implement support for the original IR remote
- Enable multi-language support
