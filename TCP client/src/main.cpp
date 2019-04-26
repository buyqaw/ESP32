#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "BuyQaw.TECH";
const char* password =  "YouWillNeverWorkAlone";

const uint16_t port = 3333;
const char * host = "192.168.1.68";

void setup()
{

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }

  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());

}

void loop()
{
    WiFiClient client;

    if (!client.connect(host, port)) {

        Serial.println("Connection to host failed");

        delay(1000);
        return;
    }

    Serial.println("Connected to server successful!");

    client.print("Hello from ESP32!");

    Serial.println("Disconnecting...");
    client.stop();

    delay(1000);
}
