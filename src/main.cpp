#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WifiManager.h>
#include <user_interface.h>
#include <FastLED.h>
#include <string>
using namespace std;

#define NUM_LEDS 2
#define DATA_PIN D3
#define CLOCK_PIN D4
#define LED_TYPE WS2801

CRGB leds[NUM_LEDS];

const int ledPin = LED_BUILTIN;
const uint16_t ledCount = 2;
const int dataPin = 10;
const int clockPin = 9;
const string lamp_name = "Lámpara de ejemplo";
unsigned long millis_last_ping = 0, millis_current = 0, millis_color_changed = 0;
const int update_interval = 10000, shutdown_interval = 60000; // 10 million miliseconds, or 10 seconds to ping, or 60 seconds to shutdown
IPAddress multicast_group(239, 255, 255, 128);
int port_mcast = 11555;
int port_unicast = 11556;
WiFiUDP wifiudp;
WiFiManager wifiManager;
char packetBuffer[256];
string msg;
string ping_multicast;


void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);

  // Initialize the LED strip

  // Set builtin pin to output, and set it to HIGH
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  // Initialize Serial Communication
  Serial.begin(9600);

  // Set WiFiManager timeout to 120 seconds
  // After this time, the ESP will enter AP mode
  wifiManager.setTimeout(120);

  // Attempt to connect to WiFi network
  if(!wifiManager.autoConnect("ESP", "123")){
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  // If we connected, print out the IP address
  Serial.println("Conectado!");

  Serial.println("local ip");

  Serial.println(WiFi.localIP());

  // Set ping message
  ping_multicast = "Lamp," + lamp_name + "," + WiFi.localIP().toString().c_str();

  // Initialize UDP listener
  wifiudp.begin(port_unicast);
}

void blink(int ms)
{
  digitalWrite(ledPin, LOW);
  delay(ms/2);
  digitalWrite(ledPin, HIGH);
  delay(ms/2);
}

void loop() { 

  // Listen for incoming UDP packets
  int packetSize = wifiudp.parsePacket();

  // If a packet was received
  if(packetSize){
    // Print out the packet
    Serial.println("Packet received");
    blink(200);
    int packetLen = wifiudp.read(packetBuffer, 256);
    if(packetLen > 0){
      // Terminate the string, setting the last character to 0
      packetBuffer[packetSize] = 0;
      Serial.println(packetBuffer);

      // Convert the packet to a string to be able to use it
      msg = packetBuffer;

      if (msg.length() > 5 && msg.substr(0, 5) == "Color") {
        // Extract the color from the message
        string color = "0x" + msg.substr(msg.find("#")+1);

        Serial.println(color.c_str());
        // Convert the color to an integer from base 16
        uint32 color_int = strtol(color.c_str(), NULL, 16);
        // Set the LED to the color and show it
        leds[0] = color_int;
        FastLED.show();

        // Update the time of the last color change
        millis_color_changed = millis();
      }
    }
  }

  // Update actual running time
  millis_current = millis();

  if ((unsigned long)(millis_current - millis_last_ping) >= update_interval) {
    // Send a ping to the multicast group every 10 seconds
    Serial.println("Sending ping");

    wifiudp.beginPacket(multicast_group, port_mcast);
    wifiudp.write(ping_multicast.c_str());
    wifiudp.endPacket();

    // Update the last ping time
    millis_last_ping = millis_current;
  }

  if ((unsigned long)(millis_current - millis_color_changed) >= shutdown_interval) {
    // Set the LED to black and show it if it has not been changed in the last 60 seconds
    leds[0] = CRGB::Black;
    FastLED.show();
  }

}
