## Install Arduino GUI v2.0

- [ ] Download the installer from www.arduino.com
- [ ] Add the ESP32 HW Description by open `File` -> `Preferences` and add into additional boards manager URLs: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json<img width="982" alt="image" src="https://github.com/digitalsputnik/ApolloHardware/assets/37544886/5657fce7-1a58-4c83-8b3b-cd56f81d641f">

from here on Arduino should compile and upload to the Apollo (or any other ESP32) device
yet for some strange reason I got this error

[insert screenshot of the error here]

To fix this I needed to copy all the headers from here:
```
C:\Users\Virtual Production 2\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\esp-2021r2-patch5-8.4.0\lib\gcc\xtensa-esp32-elf\8.4.0\include
```
to here:
```
:\Users\Virtual Production 2\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\esp-2021r2-patch5-8.4.0\xtensa-esp32-elf\include\c++\8.4.0\bits
```
