/*#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>*/


//#define DEBUG_ESP_PORT Serial

//neotestovaná změna: 4 podmínky pro odpověd arduinu s mac addressou

#include <Arduino.h>
#include "UDPcommunication.h"
#include "helper.h"


UDPcommunication* UDPgreenhouse = new UDPcommunication();

const char *ssid;
const char *pass;
bool first = true;
//======================================================================
// Setup
//=======================================================================

//websocket variables
WebSocketClient webSocketClient;
WiFiClient client;
bool WebSocketConnected = false;
String status = "unready";

void listenForEspCommandsDuringStartup() {
      if(Serial.available()) {
      DEBUG_MSG_LN("Sending data");
      String data = Serial.readStringUntil('\n');
      std::vector<String> command = getParsedCommand(data);
      if(command[0] == "getMac\r") {
        if(command.size() == 1) {
          Serial.println("res|"+WiFi.macAddress());
        }
      }
      if(command[0] == "getStatus\r") {
        if(command.size() == 1) {
          Serial.println("res|"+status);
        }
      }
    }
}

void setup()
{
  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(Serial);
  Serial.println();
  WiFi.setAutoConnect(true);
  delay(100);
  delay(1000);
}
bool needSetup = false;
//======================================================================
// MAIN LOOP
//======================================================================
void loop()
{
  if(WiFi.isConnected() || UDPgreenhouse->authSuccess()){
    //======================================================================
    // CONNECTION ESTABLISHED
    //======================================================================
    if(UDPgreenhouse->authSuccess()){
      //WiFi.mode(WIFI_STA);
      if(needSetup){
        UDPgreenhouse->stopListenForCredentials();
        needSetup = false;
      }
      //======================================================================
      // HUB LOCATED ON SITE
      //======================================================================
      if(UDPgreenhouse->haveHub()) {
        if(WebSocketConnected && client.connected()){
          //======================================================================
          // WEBSOCKET CONNECTED WITH HUB
          //======================================================================
          DEBUG_MSG_LN("websocket listenning");

          if(first) {
            status = "ready";
            first = false;
            webSocketClient.sendData("sendMac|"+WiFi.macAddress());
          }

          String data;
          webSocketClient.getData(data);
          if (data.length() > 0) {
            DEBUG_MSG_LN("Received data");
            //std::vector<String> response = getParsedCommand(data);
            Serial.println(data);
          }
          
          if(Serial.available()) {
            DEBUG_MSG_LN("Sending data");
            String data = Serial.readStringUntil('\n');
            std::vector<String> command = getParsedCommand(data);
            if(command[0] == "getMac\r") {
              if(command.size() == 1) {
                Serial.println("res|"+WiFi.macAddress());
              }
            } else if(command[0] == "getStatus\r") {
              if(command.size() == 1) {
                Serial.println("res|"+status);
              }
            } else {
              webSocketClient.sendData(data);
            }
          }

        } else {
          //======================================================================
          // WEBSOCKET NOT CONNECTED
          //======================================================================

          if(!first) {
              DEBUG_MSG_LN("Connection Lost");
              status = "lost";
              first = true;
          }
          listenForEspCommandsDuringStartup();

          if(!client.connected()) {
            if (client.connect(UDPgreenhouse->hubIP.toString(), 3000)) {
              DEBUG_MSG_LN("Connected");
              WebSocketConnected = false;
            } else {
              DEBUG_MSG("Connection failed");
            }

          } else {
            webSocketClient.path = "/";
            String ip = UDPgreenhouse->hubIP.toString().c_str(); //https://stackoverflow.com/questions/7352099/stdstring-to-char
            char *copyIP = new char[ip.length() + 1];
            strcpy(copyIP, ip.c_str());
            webSocketClient.host = copyIP;
            delete [] copyIP;
            if (webSocketClient.handshake(client)) {
                DEBUG_MSG_LN("Handshake successful");
                WebSocketConnected = true;
              } else {
                DEBUG_MSG_LN("Handshake failed.");
              }
          }
        }


        DEBUG_MSG("connected with IP: ");
        DEBUG_MSG_LN(WiFi.localIP().toString().c_str());
        DEBUG_MSG("and have hub on IP: ");
        DEBUG_MSG_LN(UDPgreenhouse->hubIP.toString().c_str());

        delay(1000);
      } else {
        //======================================================================
        // LISTEN FOR HUB RESPONSE
        //======================================================================
        listenForEspCommandsDuringStartup();

        char* hubip = UDPgreenhouse->listenForHub();
        if(hubip){
          DEBUG_MSG(hubip);
          delete hubip;
        }
      }
    }
    //======================================================================
    // CONNECTION FAILED OR OTHER EVENTS
    //======================================================================
    else if(UDPgreenhouse->authFailed()){
      DEBUG_MSG("bad credentials");
      DEBUG_MSG_LN(WiFi.SSID());
      WiFi.disconnect();
    }else if(UDPgreenhouse->changingStatus()){
      DEBUG_MSG_LN("changing status");
    } else if(UDPgreenhouse->disconnected()) {
      DEBUG_MSG_LN("disconnected");
    }
    //test.check();
  } else {

    listenForEspCommandsDuringStartup();

    //======================================================================
    // FIRST TIME SETUP
    //======================================================================
    needSetup = true;
    char* credentials = UDPgreenhouse->listenForCredentials();
    if(credentials){
      if(String(subStr(credentials,"|",1))=="WiFi-login"){
        String name = String(subStr(credentials,"|",2));
        String password = String(subStr(credentials,"|",3));
        //DEBUG_MSG(name+" "+password);
        DEBUG_MSG_LN("recived login credentials");
        UDPgreenhouse->connect(name,password);
      }
      delete credentials;
    }
  }
}
