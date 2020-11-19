#ifndef UC
#define UC

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "WebSocketClient.h"


#if(ARDUINO >= 100)
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#define DEBUG_ESP_PORT Serial

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#define DEBUG_MSG_LN(...) DEBUG_ESP_PORT.println( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#define DEBUG_MSG_LN(...)
#endif
  
class UDPcommunication {
  public:
    // Constructor
    UDPcommunication();
    
    // Methods
    void connect(String ssid,String pass);
    char* listenForCredentials();
    void stopListenForCredentials();
    bool authSuccess();
    bool authFailed();
    bool changingStatus();
    bool disconnected();
    bool haveHub();
    IPAddress hubIP;
    char* listenForHub();
    void check();

  private:
    char* _ssid;
    char* _pass;
    bool APRunning = false;
    bool UDPRunning = false;
    int _localPort = 12345;
    int _publicPort = 2551;
    IPAddress serverIP;
    IPAddress brodcastIP;
    IPAddress gatewayIP;
    IPAddress subnetIP;
    WiFiUDP* udp;
  
};
#endif