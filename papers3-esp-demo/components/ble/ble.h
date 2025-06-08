#ifndef _BLE_H_
#define _BLE_H_

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID

// #define CHARACTERISTIC_SSID_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"      // WIFI SSID
// #define CHARACTERISTIC_PWD_UUID_RX "6E400012-B5A3-F393-E0A9-E50E24DCCA9E"       // WIFI PASSWD
// #define CHARACTERISTIC_BRIGHTNESS_RX "6E400032-B5A3-F393-E0A9-E50E24DCCA9E" // longitude
// #define CHARACTERISTIC_APIKEY_UUID_RX "6E400042-B5A3-F393-E0A9-E50E24DCCA9E"    // APIKey
// #define CHARACTERISTIC_LOCATION_UUID_RX "6E400052-B5A3-F393-E0A9-E50E24DCCA9E" // Location name
// #define CHARACTERISTIC_LANGUAGE_UUID_RX "6E400062-B5A3-F393-E0A9-E50E24DCCA9E" // Language

#define CHARACTERISTIC_UUID_RX "6E400022-B5A3-F393-E0A9-E50E24DCCA9E"  // latitude
#define CHARACTERISTIC_UUID_TX "6E4000FF-B5A3-F393-E0A9-E50E24DCCA9E"

enum
{
    RXCBIDX_RX = 0,
    // RXCBIDX_BRIGHTNESS,
    RXCBIDX_MAX
};


enum
{
    BLE_OK = 0,
    BLE_ERR_INVALID_CRC,
    BLE_ERR_INVALID_PACK_TYPE,
    BLE_ERR_INVALID_PACK_LEN,
    BLE_ERR_OTA_FAILED,
    BLE_ERR_BAD_CHECKSUM
};

void BLEStart(int face_sel);
void BLESend(uint8_t *data, size_t len);


class MyServerCallbacks : public BLEServerCallbacks
{
public:
    bool deviceConnected;
    bool oldDeviceConnected;
private:
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
};

class MyCallbacks : public BLECharacteristicCallbacks
{
public:
    bool isReady(void);
    String payload;
private:
    bool _isReady = false;
    void onWrite(BLECharacteristic *pCharacteristic);
    void onNotify(BLECharacteristic *pCharacteristic);
};

extern MyServerCallbacks* ble_server_cb;
extern MyCallbacks* ble_rx_cb[RXCBIDX_MAX];

#endif