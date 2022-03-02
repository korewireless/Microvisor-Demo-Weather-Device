# Twilio Microvisor Weather Device Demo 1.0.2

This repo provides a basic demonstration of a sample weather monitor application. It makes use of an 8x8 matrix display to periodically present the local weather conditions, which are retrieved from the [OpenWeather API](https://openweathermap.org/api/one-call-api). The OpenWeather data is parsed using [cJSON](https://github.com/DaveGamble/cJSON).

The application is based on the [FreeRTOS](https://freertos.org/) real-time operating system and which will run on the “non-secure” side of Microvisor. FreeRTOS is included as a submodule.

The [ARM CMSIS-RTOS API](https://github.com/ARM-software/CMSIS_5) is used an an intermediary between the application and FreeRTOS to make it easier to swap out the RTOS layer for another.

The application code files can be found in the [`App/`](App/) directory. The [`ST_Code/`](ST_Code/) directory contains required components that are not part of Twilio Microvisor STM32U5 HAL, which this sample code accesses as a submodule. The required `FreeRTOSConfig.h` and `stm32u5xx_hal_conf.h` configuration files are located in the [config/](config/) directory.

## Cloning the Repo

This repo makes uses of git submodules, some of which are nested within other submodules. To clone the repo, run:

```bash
git clone https://github.com/TwilioDevEd/microvisor-weather-device-demo.git
```

and then:

```bash
cd microvisor-weather-device-demo
git submodule update --init --recursive
```

## Requirements

You will need a Twilio account. [Sign up now if you don’t have one](https://www.twilio.com/try-twilio).

The demo makes use of the [OpenWeather API](https://openweathermap.org/api/one-call-api). To make use of this API, you will need to [set up an OpenWeather account](https://home.openweathermap.org/users/sign_up) and obtain an API key.

Get your location — or any location co-ordinates — from [Google Maps](https://www.google.co.uk/maps).

You will also need the following hardware:

* A Twilio Microvisor Nucleo Development Board. These are currently only available to Private Beta program participants. You will need to solder male header pins to the two GPIO banks on the board, or at the very least to the connected pins shown in the circuit diagram below.
* An HT16K33-based 8x8 matrix display, e.g., [Adafruit 1.2-inch 8x8 LED Matrix and Backpack](https://www.adafruit.com/product/1856).
* Four female-to-female jumper wires.

## Hardware Setup

Assemble the following circuit:

![The Microvisor IOT device demo circuit](./images/circuit.png)

The display are shown on breakout boards which include I2C pull-up resistors. If you add the display and sensor as raw components, you will need to add pull-ups on the I2C SDA and SCL lines. You only need a single pull-up on each line.

## Software Setup

This project is written in C. At this time, we only support Ubuntu 20.0.4. Users of other operating systems should build the code under a virtual machine running Ubuntu.

**Note** macOS users may attempt to install the pre-requisites below using [Homebrew](https://brew.sh). This is not supported, but should work. You may need to change the names of a few of the packages listed in the `apt install` command below.

### Pre-requisites

#### LIbraries

Under Ubuntu, run the following:

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi git \
                 python3 python3-pip build-essential protobuf-compiler \
                 cmake libsecret-1-dev curl jq
```

Now run:

```bash
pip3 install cryptography protobuf
```

#### Twilio CLI

Install the Twilio CLI (required to view streamed logs):

```bash
curl -o - https://raw.githubusercontent.com/nvm-sh/nvm/master/install.sh | bash
```

Close your terminal window or tab, and open a new one. Now run:

```bash
nvm install --lts
npm install twilio-cli -g
twilio plugins:install @twilio/plugin-microvisor
```

#### Environment Variables

Running the Twilio CLI and the project's [deploy script](./deploy.sh) — for uploading the built code to the Twilio cloud and subsequent deployment to your Microvisor Nucleo Board — uses the following Twilio credentials stored as environment variables. They should be added to your shell profile:

```bash
export TWILIO_ACCOUNT_SID=ACxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TWILIO_AUTH_TOKEN=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TWILIO_API_KEY=SKxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TWILIO_API_SECRET=YKxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export MV_DEVICE_SID=UVxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

You can get the first two from your Twilio Console [account dashboard](https://console.twilio.com/).

To generate API keys and secrets, visit [**Account > API keys & tokens**](https://twilio.com/console/project/api-keys/).

Restart your terminal and enter the following command:

```bash
curl https://microvisor.twilio.com/v1/Devices \
  -u ${TWILIO_ACCOUNT_SID}:${TWILIO_AUTH_TOKEN} -s | jq
```

This will yield JSON which contains a `device` array — your Microvisor Nucleo Board will be in that array. Use the value of its `sid` field for your `MV_DEVICE_SID` value.

#### OpenWeather

Before you build the app, set the following environment variables:

```bash
export MVOW_API_KEY="<YOUR_OPEN_WEATHER_API_KEY>"
export MVOW_LAT=<YOUR_LATITUDE_CO-ORDINATE>
export MVOW_LNG=<YOUR_LONGITUDE_CO-ORDINATE>
```

## Build the Application Demo Code

Build the sample code with:

```bash
cd microvisor-weather-device-demo
cmake -S . -B build/
cmake --build build --clean-first
```

## Deploy the Application

Run:

```bash
./deploy.sh --log
```

This will upload the build and stage it for deployment to your device. If you encounter errors, please check your stored Twilio credentials.

The `--log` flag initiates log-streaming.

## View Log Output

You can start log streaming separately — for example, in a second terminal window — with this command:

```bash
twilio microvisor:logs:stream ${MV_DEVICE_SID}
```

## Copyright and Licensing

The sample code and Microvisor SDK is © 2022, Twilio, Inc. It is licensed under the terms of the [Apache 2.0 License](./LICENSE).

The SDK makes use of code © 2021, STMicroelectronics and affiliates. This code is licensed under terms described in [this file](https://github.com/twilio/twilio-microvisor-hal-stm32u5/blob/main/LICENSE-STM32CubeU5.md).

The SDK makes use [ARM CMSIS](https://github.com/ARM-software/CMSIS_5) © 2004, ARM. It is licensed under the terms of the [Apache 2.0 License](./LICENSE).

[FreeRTOS](https://freertos.org/) is © 2021, Amazon Web Services, Inc. It is licensed under the terms of the [Apache 2.0 License](./LICENSE).

[cJSON](https://github.com/DaveGamble/cJSON) is © 2009-2017 Dave Gamble and cJSON contributors. It is licensed under the terms of the [MIT License](./LICENSE.md).
