#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define NUM_LEDS 149
#define BUFFER_LEN 64
#define LED_PIN 4
#define MyApiKey "e6b21115-e8a2-404f-980a-0b0833cf4ad7"
#define MySSID "JustANet"
#define MyWifiPassword "wifi4you"

CRGB leds[NUM_LEDS];
char packetBuffer[BUFFER_LEN];
uint8_t N = 0;
WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
uint32_t tmr = 0;
byte i, brDesk, brBed, brMain;
byte rBed,  gBed,  bBed;
byte rDesk, gDesk, bDesk;
byte rMain, gMain, bMain;
boolean deskIsOn, bedIsOn, mainIsOn, playing = false, updReq = false;



void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

void setup() {
    Serial.begin(115200);
    EEPROM.begin(15); //we need only 15 bytes
    deskIsOn = (EEPROM.read(0) > 0);
    bedIsOn = (EEPROM.read(1) > 0);
    mainIsOn = (EEPROM.read(2) > 0);
    brDesk = EEPROM.read(3);
    brBed = EEPROM.read(4);
    brMain = EEPROM.read(5);
    if(brDesk > 100) brDesk = 100;
    if(brBed > 100) brBed = 100;
    if(brMain > 100) brMain = 100;
    rBed = EEPROM.read(6);
    gBed = EEPROM.read(7);
    bBed = EEPROM.read(8);
    rDesk = EEPROM.read(9);
    gDesk = EEPROM.read(10);
    bDesk = EEPROM.read(11);
    rMain = EEPROM.read(12);
    gMain = EEPROM.read(13);
    bMain = EEPROM.read(14);
    FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS); 
    for(i = 114; i < 149; i++){
        if(deskIsOn) leds[i].setRGB( byte(gDesk*brDesk/100), byte(rDesk*brDesk/100), byte(bDesk*brDesk/100));
        else leds[i] = CRGB::Black;
    }
    for(i = 14; i < 114; i++){
        if(mainIsOn) leds[i].setRGB( byte(gMain*brMain/100), byte(rMain*brMain/100), byte(bMain*brMain/100));
        else leds[i] = CRGB::Black; 
    }
    for(i = 0; i < 14; i++){
        if(bedIsOn) leds[i].setRGB( byte(gBed*brBed/100), byte(rBed*brBed/100), byte(bBed*brBed/100));
        else leds[i] = CRGB::Black; 
    }
    FastLED.show();
    WiFiMulti.addAP(MySSID, MyWifiPassword);
    // Waiting for Wifi connect
    while (WiFiMulti.run() != WL_CONNECTED) {
        delay(500);
    }
    webSocket.begin("iot.sinric.com", 80, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setAuthorization("apikey", MyApiKey);
    webSocket.setReconnectInterval(5000);
}


void loop() {
    webSocket.loop();
    int packetSize = Serial.available();
    // If packets have been received, interpret the command
    if (packetSize) {
        if(!playing){
            playing = true;
            for(i = 14; i < 114; i++){
                leds[i] = CRGB::Black; 
            }
        }
        int len = Serial.readBytes(packetBuffer, BUFFER_LEN);
        for(int i = 0; i < len; i+=4) {
            packetBuffer[len] = 0;
            N = packetBuffer[i];
            leds[N + 14].setRGB((uint8_t)packetBuffer[i+2], (uint8_t)packetBuffer[i+1], (uint8_t)packetBuffer[i+3]);
        }
        if(updReq){
          updReq = false;
          for(i = 114; i < 149; i++){
            if(deskIsOn) leds[i].setRGB( byte(gDesk*brDesk/100), byte(rDesk*brDesk/100), byte(bDesk*brDesk/100));
            else leds[i] = CRGB::Black;
          }
          for(i = 0; i < 14; i++){
            if(bedIsOn) leds[i].setRGB( byte(gBed*brBed/100), byte(rBed*brBed/100), byte(bBed*brBed/100));
            else leds[i] = CRGB::Black; 
          }
        }
        FastLED.show();
        tmr = millis();
    } else if(playing && (millis() - tmr > 400)){
        for(i = 14; i < 114; i++){
          if(mainIsOn) leds[i].setRGB( byte(gMain*brMain/100), byte(rMain*brMain/100), byte(bMain*brMain/100));
          else leds[i] = CRGB::Black; 
        }
        FastLED.show(); 
        playing = false;
    }
    if(updReq){
      updReq = false;
      for(i = 114; i < 149; i++){
        if(deskIsOn) leds[i].setRGB( byte(gDesk*brDesk/100), byte(rDesk*brDesk/100), byte(bDesk*brDesk/100));
        else leds[i] = CRGB::Black;
      }
      for(i = 0; i < 14; i++){
        if(bedIsOn) leds[i].setRGB( byte(gBed*brBed/100), byte(rBed*brBed/100), byte(bBed*brBed/100));
        else leds[i] = CRGB::Black; 
      }
      for(i = 14; i < 114; i++){
        if(mainIsOn) leds[i].setRGB( byte(gMain*brMain/100), byte(rMain*brMain/100), byte(bMain*brMain/100));
        else leds[i] = CRGB::Black; 
      }
      FastLED.show(); 
    }
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT){
    updReq = true;
    DynamicJsonDocument json(1024);
    deserializeJson(json, (char*) payload);
    String deviceId = json ["deviceId"];
    String action = json ["action"];
    if (deviceId == "5e5bfab7a23b266b59a9c7f0"){ // Device ID of the Desk Lamp
      if (action == "action.devices.commands.OnOff") {
        String value = json ["value"]["on"];
        if(value == "true"){
          deskIsOn = true;
          EEPROM.write(0, 255);
          EEPROM.commit();
        } else{
          deskIsOn = false;
          EEPROM.write(0, 0);
          EEPROM.commit();
        }
      } else if (action == "action.devices.commands.BrightnessAbsolute") {
          brDesk = byte(int(json["value"]["brightness"]));
          EEPROM.write(3, brDesk);
          EEPROM.commit();
      } else if (action == "action.devices.commands.ColorAbsolute") {
          uint32_t v = int(json["value"]["color"]["spectrumRGB"]);
          rDesk = v >> 16;
          gDesk = v >> 8 & 0xFF;
          bDesk = v & 0xFF;
          EEPROM.write(9, rDesk);
          EEPROM.write(10, gDesk);
          EEPROM.write(11, bDesk);
          EEPROM.commit();
      }
    }
    else if (deviceId == "5e5bfae9a23b266b59a9c7fb"){ // Device ID of the Main Lamp
      if (action == "action.devices.commands.OnOff") {
        String value = json ["value"]["on"];
        if(value == "true"){
          mainIsOn = true;
          EEPROM.write(2, 255);
          EEPROM.commit();
        } else{
          mainIsOn = false;
          EEPROM.write(2, 0);
          EEPROM.commit();
        }
      } else if (action == "action.devices.commands.BrightnessAbsolute") {
          brMain = byte(int(json["value"]["brightness"]));
          EEPROM.write(5, brMain);
          EEPROM.commit();
      } else if (action == "action.devices.commands.ColorAbsolute") {
          uint32_t v = int(json["value"]["color"]["spectrumRGB"]);
          rMain = v >> 16;
          gMain = v >> 8 & 0xFF;
          bMain = v & 0xFF;
          EEPROM.write(12, rMain);
          EEPROM.write(13, gMain);
          EEPROM.write(14, bMain);
          EEPROM.commit();
      }
    }
    else if (deviceId == "5e5bfa53a23b266b59a9c7d9"){ // Device ID of the Bedside Lamp
      if (action == "action.devices.commands.OnOff") {
        String value = json ["value"]["on"];
        if(value == "true"){
          bedIsOn = true;
          EEPROM.write(1, 255);
          EEPROM.commit();
        } else{
          bedIsOn = false;
          EEPROM.write(1, 0);
          EEPROM.commit();
        }
      } else if (action == "action.devices.commands.BrightnessAbsolute") {
          brBed = byte(int(json["value"]["brightness"]));
          EEPROM.write(4, brBed);
          EEPROM.commit();
      } else if (action == "action.devices.commands.ColorAbsolute") {
          uint32_t v = int(json["value"]["color"]["spectrumRGB"]);
          rBed = v >> 16;
          gBed = v >> 8 & 0xFF;
          bBed = v & 0xFF;
          EEPROM.write(6, rBed);
          EEPROM.write(7, gBed);
          EEPROM.write(8, bBed);
          EEPROM.commit();
      } 
    }
  }
}
