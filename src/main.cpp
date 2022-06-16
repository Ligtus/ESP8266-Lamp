#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WifiManager.h>
#include <user_interface.h>
#include <FastLED.h>
#include <string>
using namespace std;

#define NUM_LEDS 2
#define RESET_PIN D2
#define DATA_PIN D3
#define CLOCK_PIN D5
#define LED_TYPE WS2801

CRGB leds[NUM_LEDS];

const uint16_t ledCount = 2;
const int dataPin = 10;
const int clockPin = 9;
const string lamp_name = "Lámpara de ejemplo";
unsigned long millis_last_ping = 0, millis_current = 0, millis_color_changed = 0;
const int ping_interval = 10000, shutdown_interval = 60000, update_interval = 1000; // 10 million miliseconds, or 10 seconds to ping, or 60 seconds to shutdown
IPAddress multicast_group(239, 255, 255, 128);
int port_mcast = 11555;
int port_unicast = 11556;
WiFiUDP wifiudp;
WiFiManager wifiManager;
char packetBuffer[256];
string msg;
string ping_multicast;
boolean fading = false;
CRGB new_color_rgb;
TBlendType blend_type = LINEARBLEND;
int current_brightness = 0;

void setup() {
  // Initialize the LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);

  // Set reset pin to input
  pinMode(RESET_PIN, INPUT_PULLUP);

  // Set builtin pin to output, and set it to HIGH
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  wifiManager.autoConnect();

  // Initialize Serial Communication
  Serial.begin(9600);

  Serial.println(WiFi.localIP());

  // Set ping message
  ping_multicast = "Lamp," + lamp_name + "," + WiFi.localIP().toString().c_str();

  // Initialize UDP listener
  wifiudp.begin(port_unicast);

  // Set LEDs to black
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void loop() { 

  // Check if the reset button is pressed
  if(digitalRead(RESET_PIN) == LOW) {
    wifiManager.startConfigPortal("Example lamp", "12345678");
    Serial.println("Access Point connected...");
  }

  // Update actual running time
  millis_current = millis();

  // Listen for incoming UDP packets
  int packetSize = wifiudp.parsePacket();

  // If a packet was received
  if(packetSize){
    // Print out the packet
    Serial.println("Packet received");
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

        new_color_rgb = CRGB(color_int);
      }
    }
  }
  
  if (!fading && leds[0] != new_color_rgb) {
    EVERY_N_MILLIS(2) {
      leds[0] = nblend(leds[0], new_color_rgb, 8);
      if (millis_current - millis_color_changed > update_interval) {
        leds[0] = new_color_rgb;
        millis_color_changed = millis_current;
      }
      fill_solid(leds, NUM_LEDS, leds[0]);
      FastLED.show();
    }
  }
  

  if ((unsigned long)(millis_current - millis_last_ping) >= ping_interval) {
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
    EVERY_N_MILLIS(15) {
      fading = true;
      fadeToBlackBy(leds, NUM_LEDS, 10);
      FastLED.show();
    }

    if (leds[0] == CRGB(0,0,0)) {
      // If the LED is black, turn it off
      new_color_rgb = CRGB::Black;
      fading = false;
    }
  }
}
