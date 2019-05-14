# Arduino-RGBLamp

This repository is about a nice lamp I made for my desk. My lamp features several modes, including rainbow, study, reading and disco mode.
Those modes can be simply changed by just pressing a button.
The project is based on a ESP (Node MCU v1) and a WS2812 LED strip. 
Any other Arduino should be working for this project, too.

## Part List

- [ESP Node MCU v1](https://aliexpress.com/item/ESP8266-CH340G-CH340-G-NodeMcu-V3-Lua-Wireless-WIFI-Module-Connector-Development-Board-Based-ESP-12E/32800966224.html)

- [Microphone](https://aliexpress.com/item/Sound-Detector-module/32569653599.html?spm=a2g0s.9042311.0.0.78634c4dSSgc9y)

- [1m 144LEDs WS2812 light strip](https://aliexpress.com/item/1m-2m-3m-4m-5m-ws2812b-ws2812-led-strip-individually-addressable-smart-led-strip-black-white/32682015405.html?spm=a2g0s.9042311.0.0.78634c4dSSgc9y)

- [Basic Push Button](https://www.aliexpress.com/item/1Pcs-2Pin-Mini-Switch-12mm-1A-waterproof-switch-12v-momentary-Push-button-Switch-since-the-reset/32833295404.html?spm=a2g0x.10010108.1000001.12.61582925XHdq1K&ws_ab_test=searchweb0_0%2Csearchweb201602_5_10065_10130_10068_10547_319_317_10548_10696_453_10084_454_10083_10618_10307_537_536_10131_10132_10133_10059_10884_10887_321_322_10103%2Csearchweb201603_52%2CppcSwitch_0&algo_pvid=ef1e3f05-f70e-4ceb-a506-3c1815559463&algo_expid=ef1e3f05-f70e-4ceb-a506-3c1815559463-2)

- 1kΩ Resistor for connecting the push button
- 300Ω - 500Ω Resistor for the LED strip data link
- 1000μF Capacitor

## Wiring
&nbsp; 

![Fritzing](https://github.com/fabianh001/Arduino-RGBLamp/blob/master/fritzing.png)

| ESP Pin       | Description      
|:-------------:|:----------:|
| Vin | 5v power in          |
| GND | connected to ground  |
| A0  | microphone data in   |
| D6  | push button in       |
| D1  | LED strip data out   |   

## Code
For this project you need to download and install the [FastLED](https://github.com/FastLED/FastLED) library. Refer to the library's [ESP notes](https://github.com/FastLED/FastLED/wiki/ESP8266-notes) to get the library properly working with an ESP.

The original repository I used for the lamp's disco mode is hansjny's [sound reactive LED strip project](https://github.com/hansjny/Natural-Nerd/blob/master/arduino/soundsread2/sound_reactive.ino). Don't forget to check it out.

