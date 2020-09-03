/*#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>*/


//#define DEBUG_ESP_PORT Serial

#include <Arduino.h>
#include "UDPcommunication.h"
#include "helper.h"


UDPcommunication* test = new UDPcommunication();

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

void setup()
{
  delay(5000);


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
  if(WiFi.isConnected() || test->authSuccess()){
    //======================================================================
    // CONNECTION ESTABLISHED
    //======================================================================
    if(test->authSuccess()){
      //WiFi.mode(WIFI_STA);
      if(needSetup){
        test->stopListenForCredentials();
        needSetup = false;
      }

      if(test->haveHub()) {
        if(WebSocketConnected && client.connected()){
          DEBUG_MSG_LN("websocket listenning");

          if(first) {
            Serial.println("ready");
            first = false;
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
            webSocketClient.sendData(Serial.readString());
          }

        } else {
          if(!client.connected()) {
            if (client.connect(test->hubIP.toString(), 3000)) {
              DEBUG_MSG_LN("Connected");
              WebSocketConnected = false;
            } else {
              DEBUG_MSG_LN("Connection failed.");

              Serial.println("lost");
              first = true;
              
            }
          } else {
            webSocketClient.path = "/";
            String ip = test->hubIP.toString().c_str(); //https://stackoverflow.com/questions/7352099/stdstring-to-char
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
        DEBUG_MSG_LN(test->hubIP.toString().c_str());

        delay(1000);
      } else {
        char* hubip = test->listenForHub();
        if(hubip){
          DEBUG_MSG(hubip);
          delete hubip;
        }
      }
    }
    //======================================================================
    // CONNECTION FAILED OR OTHER EVENTS
    //======================================================================
    else if(test->authFailed()){
      DEBUG_MSG("bad credentials");
      DEBUG_MSG_LN(WiFi.SSID());
      WiFi.disconnect();
    }else if(test->changingStatus()){
      DEBUG_MSG_LN("changing status");
    } else if(test->disconnected()) {
      DEBUG_MSG_LN("disconnected");
    }
    //test.check();
  } else {
    //======================================================================
    // FIRST TIME SETUP
    //======================================================================
    needSetup = true;
    char* credentials = test->listenForCredentials();
    if(credentials){
      if(String(subStr(credentials,"|",1))=="WiFi-login"){
        String name = String(subStr(credentials,"|",2));
        String password = String(subStr(credentials,"|",3));
        //DEBUG_MSG(name+" "+password);
        DEBUG_MSG_LN("recived login credentials");
        test->connect(name,password);
      }
      delete credentials;
    }
  }
}
