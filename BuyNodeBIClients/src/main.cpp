#include <Arduino.h>
#include "BLEDevice.h"
//#include "BLEScan.h"


// The remote service we wish to connect to.
static BLEUUID    serviceUUID("ba10");

// The characteristic of the remote service we are interested in.
static BLEUUID    checkUUID("ba52");

static BLEClient*  pClient;

static BLEScan* pBLEScan;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;

static BLEAdvertisedDevice* myDevice;

static BLERemoteCharacteristic* Check;

static boolean PassOK = false;
static boolean PassOK0 = false;
static boolean PassOK2 = false;
static boolean PassOK3 = false;
static boolean Yes = false;


static void HODOR(){
  Serial.println("Door is opened");
  delay(7000);
}

static void PasswdCheck(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print("Password UUID");
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
    //
    // Serial.print("Yes is: ");
    // Serial.println(Yes);
    // Serial.print("OK is: ");
    // Serial.println(PassOK);
    // Serial.print("OK2 is: ");
    // Serial.println(PassOK2);
    // Serial.print("OK3 is: ");
    // Serial.println(PassOK3);

    char password[] = "060593";

    if (PassOK0 == false){
    Yes = true;
    Serial.println("Checking password");
    for (int i = 0; i < strlen(password); i++) {
      if(password[i] != (char)pData[i]){
        Yes = false;
        Serial.println("Password is not the same");
        ESP.restart();
      }
    }
      PassOK0 = true;
  }

  if(PassOK2 == true){
    Serial.println("Password is ok, go to action");
    PassOK2 = false;
    PassOK3 = true;
  }

    if(Yes == true){
      Serial.println("Password checked, it is ok");
      Yes = false;
      PassOK = true;
      PassOK2 = true;
      Serial.println("Writing value");
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected");
    ESP.restart();
  }
};

bool connectToServer() {

    Serial.println("Forming a connection to ");

    BLEClient*  pClient  = BLEDevice::createClient();

    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());


    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      ESP.restart();
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    Check = pRemoteService->getCharacteristic(checkUUID);

    if (Check == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(checkUUID.toString().c_str());
      ESP.restart();
    }
    Serial.println(" - Found our characteristic");

    if(Check->canNotify()){
      Serial.println("Registered notify for Passwd");
      Check->registerForNotify(PasswdCheck);}
      else{
        ESP.restart();
      }

    Serial.println("Writing value of MAC to server");
    delay(200);
    Check->writeValue(BLEDevice::getAddress().toString().c_str(), BLEDevice::getAddress().toString().length());
    // Check->writeValue(BLEDevice::getAddress().toString().c_str());
    Serial.println("Done");
    doConnect = false;
}


/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

          int RSSIL = advertisedDevice.getRSSI();
          Serial.print("RSSI is: ");
          Serial.println(RSSIL);
          if (RSSIL >= -90) {
            Serial.println("Our device is near");
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
            }
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
} // End of setup.

// This is the Arduino main loop function.
void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      ESP.restart();
    }
  }
  if(PassOK == true){
    String a = "1";
    // uint8_t a[]={0x1};
    Check->writeValue(a.c_str());
    PassOK = false;
    Serial.println("Written");
  }
  if(PassOK3 == true){
    String a = "1";
    // uint8_t a[]={0x1};
    Check->writeValue(a.c_str());
    Serial.println("Written");
    HODOR();
    ESP.restart();
  }
  if (doScan == true){
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 1 second.
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(3, false);
  }
} // End of loop
