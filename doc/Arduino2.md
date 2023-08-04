## Install Arduino GUI v2.0

- [ ] Download the installer from www.arduino.com
- [ ] Add the ESP32 HW Description by open `File` -> `Preferences` and add into additional boards manager URLs: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json<img width="982" alt="image" src="https://github.com/digitalsputnik/ApolloHardware/assets/37544886/5657fce7-1a58-4c83-8b3b-cd56f81d641f">

- [ ] Open `Tools` -> `Boards` -> `Boards Manager` and type into search espressif
<img width="259" alt="image" src="https://github.com/KKallas/ApolloFirmware/assets/37544886/72d32b76-14c0-4d38-a7a0-9b3feb25b7fa">

- [ ] Install the toolset (takes up to 10min)
- [ ] Select correct Board (ESP32 Wrover Module) and Port, if the connection is not here check out the USBSerial.md
<img width="352" alt="image" src="https://github.com/KKallas/ApolloFirmware/assets/37544886/3d9c5f74-139d-4826-8410-7f2b3ad2b089">



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

To do this in Windows explorer go to your home directory and click on the address bar and type in `\appdata`
From there on all the folders are visible
## 1
<img width="560" alt="image" src="https://github.com/KKallas/ApolloFirmware/assets/37544886/88de9989-2541-4f74-b4a0-dbe03fd5acf7">

## 2
<img width="595" alt="image" src="https://github.com/KKallas/ApolloFirmware/assets/37544886/f8e637bc-f66e-42da-8a41-33823760c5f9">


