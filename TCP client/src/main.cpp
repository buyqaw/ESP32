#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SPIFFS.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Initialize Telegram BOT
#define BOTtoken "728314543:AAFJxKpaNdV_v3YsQpGFG2W6gG3GRV3cbAA"

WiFiClientSecure client1;
UniversalTelegramBot bot(BOTtoken, client1);
DynamicJsonBuffer jsonBuffer;

const char* ssid = "BuyQaw.TECH";
const char* password =  "YouWillNeverWorkAlone";

const uint16_t port = 3333;
const char * host = "192.168.1.68";
uint64_t chipid;


String data1 = "";
int scanTime = 5; //In seconds
BLEScan* pBLEScan;

#define FORMAT_SPIFFS_IF_FAILED true

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    while(file.available()){
        // Serial.write(file.read());
        data1 += char(file.read());
    }
    // Serial.println(data1);
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      String time = String(millis());
      if (SPIFFS.exists("/log.txt")) {
        File f = SPIFFS.open("/log.txt", "a");
        if (!f) {
            Serial.println("file open failed");
        } else {
            f.print(millis());
            f.print("=");
            f.printf("%s", advertisedDevice.getAddress().toString().c_str());
            f.println(";");
        }
      }
    }
};

void setup()
{

  Serial.begin(115200);
  SPIFFS.begin(true);
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
    }

    if (!SPIFFS.exists("/formatComplete.txt")) {
      Serial.println("Please wait 30 secs for SPIFFS to be formatted");
      SPIFFS.format();
      Serial.println("Spiffs formatted");

      File f = SPIFFS.open("/formatComplete.txt", "w");
      if (!f) {
          Serial.println("file open failed");
      } else {
          f.println("Format Complete");
      }
    } else {
      Serial.println("SPIFFS is formatted. Moving along...");
    }

    if (!SPIFFS.exists("/log.txt")) {
      File f = SPIFFS.open("/log.txt", "w");
      if (!f) {
          Serial.println("file open failed");
      } else {
          f.print("I started to write at: ");
          f.println(millis());
          chipid=ESP.getEfuseMac();
          f.print("My ID is:");
          f.printf("%08X\n",(uint32_t)chipid);
          f.println();
      }
    } else {
      Serial.println("Created our file");
    }


  // writeFile(SPIFFS, "/log.txt", "\r\n");
  // appendFile(SPIFFS, "/log.txt", "World!\r\n");

  // renameFile(SPIFFS, "/hello.txt", "/foo.txt");
  // readFile(SPIFFS, "/foo.txt");
  // deleteFile(SPIFFS, "/foo.txt");
  // testFileIO(SPIFFS, "/log.txt");
  // deleteFile(SPIFFS, "/test.txt");
  // Serial.println( "Test complete" );

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value

    Serial.println("Setup done");
}

void loop()
{
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  delay(5000);
  readFile(SPIFFS, "/log.txt");


  WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }

        Serial.println("Connected");
          bot.sendMessage("-1001360713730", "I am working", "");
  ESP.restart();
  //
  // int our = 0;
  // int n = WiFi.scanNetworks();
  //   Serial.println("scan done");
  //   if (n == 0) {
  //       Serial.println("no networks found");
  //   } else {
  //       Serial.print(n);
  //       Serial.println(" networks found");
  //       for (int i = 0; i < n; ++i) {
  //           // Print SSID and RSSI for each network found
  //           Serial.print(i + 1);
  //           Serial.print(": ");
  //           Serial.print(WiFi.SSID(i));
  //           Serial.print(" (");
  //           Serial.print(WiFi.RSSI(i));
  //           Serial.print(")");
  //           delay(10);
  //           if (String(WiFi.SSID(i)) == ssid){
  //             Serial.println("Found our");
  //             our = 1;
  //             break;
  //           }
  //       }
  //   }
  //   if (our == 1){
  //
  //       WiFi.begin(ssid, password);
  //       while (WiFi.status() != WL_CONNECTED) {
  //           delay(500);
  //           Serial.print(".");
  //       }
  //
  //       Serial.println("Connected");
  //
  //       if (WiFi.status() == WL_CONNECTED){
  //           WiFiClient client;
  //
  //           if (!client.connect(host, port)) {
  //
  //               Serial.println("Connection to host failed");
  //           }
  //           else{
  //             readFile(SPIFFS, "/log.txt");
  //
  //             Serial.println("Connected to server successful!");
  //             // client.print("now:");
  //             // client.println(millis());
  //             // client.println(data1);
  //
  //             bot.sendMessage("-1001360713730", data1);
  //
  //             Serial.println("Disconnecting...");
  //             // client.stop();
  //             deleteFile(SPIFFS, "/log.txt");
  //             ESP.restart();
  //           }
  //         }
  //   }
  //   else{
  //     delay(120000);
  //   }
}
