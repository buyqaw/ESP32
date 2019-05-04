#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* CheckUUID = NULL;
BLECharacteristic* PasswdUUID = NULL;
BLECharacteristic* ActionUUID = NULL;
BLECharacteristic* IncomingUUID = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

bool checking = false;

int done = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// UUID сервиса (можно выставлять только 4 значения, остальные адаптер заполнит сам)
#define SERVICE_UUID        "BA10"

// UUID характеристик
#define CHECK_UUID          "BA52" // Проверка          [WRITE]
#define PASSWD_PORT_UUID    "BA44" // Пароль            [NOTIFY]
#define ADMIN_PORT_UUID     "BA13" // Админ панель      [NOTIFY]
#define ACTION_PORT_UUID    "BA87" // Действия          [NOTIFY]
#define INCOMING_PORT_UUID  "BA76" // Прием             [WRITE]
#define FILLER_0            "BA66" // Отвлекающий       [NOTIFY]
#define FILLER_1            "BA91" // маневр, эти       [NOTIFY]
#define FILLER_2            "BA01" // характеристики    [WRITE]
#define FILLER_3            "BA28" // ничего не делают. [NOTIFY]


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected ");
    }
};


class Check: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic1) {
      std::string rxValue = pCharacteristic1->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");

        checking = true;
        std::string value = "060593";
        PasswdUUID->setValue(value);
        PasswdUUID->notify();

        Serial.println();
        Serial.println("Sended password");
        Serial.println("*********");
        Serial.println();
      }
    }
};


class Income: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic2) {
      Serial.println("Income function started");
      std::string rxValue = pCharacteristic2->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");

        if(checking = true){
          checking = false;
          Serial.println();
          Serial.println("Sending action");
          Serial.println("*********");
          Serial.println();

          std::string value1 = "1";
          ActionUUID->setValue(value1);
          ActionUUID->notify();
        }
      }
    }
};


void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("BuyQaw");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());


  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  CheckUUID = pService->createCharacteristic(
                      CHECK_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Characteristic
  PasswdUUID = pService->createCharacteristic(
                      PASSWD_PORT_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Characteristic
  ActionUUID = pService->createCharacteristic(
                      ACTION_PORT_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Characteristic
  IncomingUUID = pService->createCharacteristic(
                      INCOMING_PORT_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );


  CheckUUID->setCallbacks(new Check());
  IncomingUUID->setCallbacks(new Income());


  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  CheckUUID->addDescriptor(new BLE2902());
  PasswdUUID->addDescriptor(new BLE2902());
  ActionUUID->addDescriptor(new BLE2902());
  IncomingUUID->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // // disconnecting
    // if (!deviceConnected && oldDeviceConnected) {
    //     delay(500); // give the bluetooth stack the chance to get things ready
    //     pServer->startAdvertising(); // restart advertising
    //     Serial.println("start advertising");
    //     oldDeviceConnected = deviceConnected;
    // }
    // // connecting
    // if (deviceConnected && !oldDeviceConnected) {
    //     // do stuff here on connecting
    //     oldDeviceConnected = deviceConnected;
    // }
}
