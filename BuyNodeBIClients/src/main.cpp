#include <Arduino.h>
#include "BLEDevice.h"
//#include "BLEScan.h"


// The remote service we wish to connect to.
static BLEUUID    serviceUUID("ba10");

// The characteristic of the remote service we are interested in.
static BLEUUID    checkUUID("ba52");
static BLEUUID    passwdUUID("ba44");
static BLEUUID    adminUUID("ba13");
static BLEUUID    actionUUID("ba87");
static BLEUUID    incomingUUID("ba76");

static BLEClient*  pClient;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice* myDevice;

static BLERemoteCharacteristic* Check;
static BLERemoteCharacteristic* Passwd;
static BLERemoteCharacteristic* Admin;
static BLERemoteCharacteristic* Action;
static BLERemoteCharacteristic* Incoming;

static boolean PassOK = false;

static void HODOR(){
  Serial.println("Door is opened");
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

    char password[] = "060593";
    boolean Yes = true;

    Serial.println("Checking password");
    for (int i = 0; i < strlen(password); i++) {
      if(password[i] != (char)pData[i]){
        Yes = false;
        Serial.println("Password is not the same");
        break;
      }
    }
    if(Yes == true){
      PassOK = true;
      Serial.println("Writing value to Incoming PORT");
      String a = "1";
      // uint8_t a[]={0x1};
      Incoming->writeValue(a.c_str());
      Serial.println("Written");
    }
}

static void DoAction(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print("Action UUID");
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);

    if(PassOK == true){
      PassOK = false;
      HODOR();
      Incoming->writeValue("1", 1);
      pClient->disconnect();
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


    pClient->setClientCallbacks(new MyClientCallback());


    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    Check = pRemoteService->getCharacteristic(checkUUID);
    Passwd = pRemoteService->getCharacteristic(passwdUUID);
    Admin = pRemoteService->getCharacteristic(adminUUID);
    Action = pRemoteService->getCharacteristic(actionUUID);
    Incoming = pRemoteService->getCharacteristic(incomingUUID);

    if (Check == nullptr || Passwd == nullptr || Action == nullptr || Incoming == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(checkUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    if(Passwd->canNotify()){
      Serial.println("Registered notify dor Passwd");
      Passwd->registerForNotify(PasswdCheck);}

    // if(Admin->canNotify()){
    //   Admin->registerForNotify(PasswdCheck);}

    if(Action->canNotify()){
      Serial.println("Registered notify dor Action");
      Action->registerForNotify(DoAction);}

    Serial.println("Writing value of MAC to server");
    // Check->writeValue(BLEDevice::getAddress().toString().c_str(), BLEDevice::getAddress().toString().length());
    Check->writeValue(BLEDevice::getAddress().toString().c_str());
    Serial.println("Done");
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
          if (RSSIL >= -70) {
            Serial.println("Our device is near");
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
            }
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  Serial.println("Forming a connection to ");

  BLEClient*  pClient  = BLEDevice::createClient();

  Serial.println(" - Created client");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 1 second.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(1, false);


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
    doConnect = false;
  }
  //
  // Serial.println("New scan: ");
  // BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  // delay(100);
} // End of loop
