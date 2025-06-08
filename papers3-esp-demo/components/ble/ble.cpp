#include "ble.h"

#define TAG "BLE"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic[RXCBIDX_MAX];
String rxUUIDBuff[RXCBIDX_MAX] = {
    CHARACTERISTIC_UUID_RX,
    // CHARACTERISTIC_BRIGHTNESS_RX,
};

MyServerCallbacks* ble_server_cb;
MyCallbacks* ble_rx_cb[RXCBIDX_MAX];
BLEService *pService;
BLEAdvertising *pAdvertising;

void MyServerCallbacks::onConnect(BLEServer *pServer)
{
    printf("BLE connected\n");
    this->deviceConnected = true;
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    printf("BLE disconnected\n");
    this->deviceConnected = false;

    pServer->startAdvertising();
}

void MyCallbacks::onNotify(BLECharacteristic *pCharacteristic) {
      uint8_t* pData;
      std::string value = pCharacteristic->getValue();
      int len = value.length();
      pData = pCharacteristic->getData();
      if (pData != NULL) {
        //        Serial.print("Notify callback for characteristic ");
        //        Serial.print(pCharacteristic->getUUID().toString().c_str());
        //        Serial.print(" of data length ");
        //        Serial.println(len);
        // printf("BLE TX  ");
        // for (int i = 0; i < len; i++) {
        //   printf("%02X ", pData[i]);
        // }
        // printf("\n");
      }
    }

void BLESend(uint8_t *data, size_t len)
{
    pTxCharacteristic->setValue(data, len);
    pTxCharacteristic->notify();
}

void BLESendResult(uint8_t err_code)
{
    uint8_t data[2] = {0, err_code};
    BLESend(data, 2);
}

void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    // uint32_t start = m_millis();
    uint8_t *p = pCharacteristic->getData();
    uint32_t len = pCharacteristic->getLength();

    // ESP_LOGI(TAG, "pack type: %d", p[0]);

    // printf("recv %d bytes ----> ", len);
    // for (int i = 0; i < len; i++) {
    //     printf("%02X, ", p[i]);
    // }
    // printf("\n");

    
    BLESendResult(BLE_OK);
}

bool MyCallbacks::isReady(void)
{
    if(_isReady)
    {
        _isReady = false;
        return true;
    }
    return false;
}


void BLEStart(int face_sel)
{
    uint8_t mac[10];
    esp_efuse_mac_get_default(mac);
    // uint64_t id = ESP.getEfuseMac();
    char buf[128];
    sprintf(buf, "Stray Robot %02X%02X", mac[5], mac[4]);
    BLEDevice::init(buf);
    printf("%s\n", buf);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    ble_server_cb = new MyServerCallbacks();
    pServer->setCallbacks(ble_server_cb);

    // Create the BLE Service
    pService = pServer->createService(BLEUUID(SERVICE_UUID), (uint32_t)30, (uint8_t)0);

    pAdvertising = pServer->getAdvertising();
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    sprintf(buf, "face:%d", face_sel);
    printf("%s\n", buf);
    oAdvertisementData.setManufacturerData(buf);
    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->setCallbacks(new MyCallbacks());
    pTxCharacteristic->addDescriptor(new BLE2902());
    pTxCharacteristic->setNotifyProperty(true);

    for (int i = 0; i < RXCBIDX_MAX; i++)
    {
        pRxCharacteristic[i] = pService->createCharacteristic(
            rxUUIDBuff[i].c_str(),
            BLECharacteristic::PROPERTY_WRITE
        );
        ble_rx_cb[i] = new MyCallbacks();
        pRxCharacteristic[i]->setCallbacks(ble_rx_cb[i]);
    }
    
    // Start the service
    pService->start();
    // Start advertising
    pAdvertising->start();
    printf("Waiting a client connection to notify...\n");
}