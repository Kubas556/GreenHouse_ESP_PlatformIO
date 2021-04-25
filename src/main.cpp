/*#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>*/




#include <Arduino.h>
#include "WiFicommunication.h"
#include <string.h>
#include <cstring>
#include "helper.h"


WiFicommunication* UDPgreenhouse = new WiFicommunication();

const char *ssid;
const char *pass;
//======================================================================
// Setup
//=======================================================================

//websocket variables
WebSocketClient webSocketClient;
WiFiClient client;
bool WebSocketConnected = false;
String status = "unready";

void listenForSerialCommands() {
       if(Serial.available()) {
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
          } else if(command[0] == "disconnectWiFi\r") { 
            WiFi.disconnect();
          } else {
            if(WebSocketConnected)
            webSocketClient.sendData(data);
          }
        }
}

bool needSetup = false;
bool clientAlreadyConnected = false;
void WiFiSetup() {
  needSetup = true;
  char* credentials = UDPgreenhouse->listenForCredentials();
  if(credentials){
    if(String(subStr(credentials,"|",1))=="WiFi-login"){
      String name = String(subStr(credentials,"|",2));
      String password = String(subStr(credentials,"|",3));
      //password = password.substring(0,password.length()-1);
      //DEBUG_MSG_LN(name+" "+password);
      DEBUG_MSG_LN("recived login credentials");
      UDPgreenhouse->connect(name,password);
    }
  }

  delete credentials;
}

void connectOrReconnectWebsocket() {
  if(!client.connected()) {
    if (client.connect(UDPgreenhouse->hubIP.toString(), 3300)) {
      DEBUG_MSG_LN("Connected to websocket");
      WebSocketConnected = false;
    } else {
      DEBUG_MSG("Connection with websocket failed");
    }
  } else {
    std::string ip = UDPgreenhouse->hubIP.toString().c_str(); //https://stackoverflow.com/questions/7352099/stdstring-to-char
    /*std::string path = ("ws://"+ip+"/");
    char* copyPath = new char[path.size()+1];
    strcpy(copyPath,path.c_str());*/
    char *copyIP = new char[ip.length() + 1];
    strcpy(copyIP, ip.c_str());
    webSocketClient.path = /*copyPath*/"/";
    webSocketClient.host = copyIP;

    if (webSocketClient.handshake(client)) {
        DEBUG_MSG_LN("Hub handshake successful");
        WebSocketConnected = true;
      } else {
        DEBUG_MSG_LN("Hub handshake failed.");
      }

      //delete [] copyPath;
      delete [] copyIP;
  }
}

void setup()
{
  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(Serial);
  WiFi.setAutoConnect(true);
  delay(1000);
}

//======================================================================
// MAIN LOOP
//======================================================================
void loop()
{
  listenForSerialCommands();
  if(UDPgreenhouse->authSuccess()){
    //======================================================================
    //WiFi CONNECTION ESTABLISHED
    //======================================================================
    
    //disable wifi setup hotspot
    if(needSetup){
      UDPgreenhouse->stopListenForCredentials();
      needSetup = false;
    }
    
    if(UDPgreenhouse->haveHub()) {
      //======================================================================
      // HUB LOCATED ON SITE
      //======================================================================
      if(WebSocketConnected && client.connected()){
        //======================================================================
        // WEBSOCKET CONNECTED WITH HUB
        //======================================================================
        DEBUG_MSG_LN("websocket listenning");

        if(!clientAlreadyConnected) {
          status = "ready";
          clientAlreadyConnected = true;
          webSocketClient.sendData("sendMac|"+WiFi.macAddress());
        }

        String data;
        webSocketClient.getData(data);
        if (data.length() > 0) {
          DEBUG_MSG_LN("Received data");
          if(data == "disconnectWiFi") {
            WiFi.disconnect();
          } else {
            //std::vector<String> response = getParsedCommand(data);
            Serial.println(data);
          }
        }

      } else {
        //======================================================================
        // WEBSOCKET NOT CONNECTED
        //======================================================================

        if(clientAlreadyConnected) {
            DEBUG_MSG_LN("Connection Lost");
            status = "lost";
            WebSocketConnected = false;
            clientAlreadyConnected = false;
        }

        connectOrReconnectWebsocket();
      }


      DEBUG_MSG("connected with IP: ");
      DEBUG_MSG_LN(WiFi.localIP().toString().c_str());
      DEBUG_MSG("and have hub on IP: ");
      DEBUG_MSG_LN(UDPgreenhouse->hubIP.toString().c_str());
    } else {
      //======================================================================
      // LISTEN FOR HUB RESPONSE
      //======================================================================

      char* hubip = UDPgreenhouse->listenForHub();
      if(hubip){
        DEBUG_MSG(hubip);
      }

      delete hubip;
    }
    
  } else {
    //======================================================================
    // CONNECTION FAILED OR OTHER EVENTS
    //======================================================================
    if(UDPgreenhouse->authFailed()){
      DEBUG_MSG("bad credentials");
      DEBUG_MSG_LN(WiFi.SSID());
      WiFi.disconnect();
    }else if(UDPgreenhouse->changingStatus()){
      //DEBUG_MSG_LN("changing status");
    } else if(UDPgreenhouse->disconnected()) {
      DEBUG_MSG_LN("disconnected");
    }

    //======================================================================
    // FIRST TIME SETUP
    //======================================================================
    WiFiSetup();
  }
}
