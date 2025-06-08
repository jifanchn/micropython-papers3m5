# M5Stack PaperS3

## How to build

This project need use IDF V4.4.x to build.

Set IDF ENV

```
git clone -b v4.4.5 --recursive https://github.com/espressif/esp-idf.git esp-idf-v4.4.5
cd esp-idf-v4.4.5/
./install.sh
```

Build Project

```
cd PaperS3-UserDemo
cd components
git clone https://github.com/espressif/arduino-esp32
cd arduino-esp32
git checkout b63f947037bece7a499faf0043636dd8531b885e
cd ..
git clone https://github.com/lovyan03/LovyanGFX.git
cd LovyanGFX
git checkout 55a0f66d9278faa596c8d51a8e8a3e537dd8f44f
cd ../..
. /path/to/esp-idf-v4.4.5/export.sh
idf.py build
idf.py flash monitor
```