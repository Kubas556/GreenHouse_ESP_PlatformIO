#include "UDPcommunication.h"

/*#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>*/

/*unsigned int localPort = 12345; // local port to listen for UDP packets

IPAddress hubIP(192,168,88,255); //UDP Broadcast IP data sent to all devicess on same network

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;*/
UDPcommunication::UDPcommunication()
{
  IPAddress brodcast(255, 255, 255, 255);
  brodcastIP = brodcast;
  //hubIP = brodcastIp;
  IPAddress espIP(192, 168, 0, 2);
  serverIP = espIP;
  IPAddress gateway(192, 168, 0, 1);
  gatewayIP = gateway;
  IPAddress subnet(255, 255, 255, 0);
  subnetIP = subnet;
  udp = new WiFiUDP();
}

void UDPcommunication::connect(String ssid, String pass)
{
  WiFi.mode(WIFI_AP_STA);
  int len1 = ssid.length();
  int len2 = pass.length();
  // declaring character array
  char ssidToChar[len1 + 1];
  char passToChar[len2 + 1];

  // copying the contents of the
  // string to char array
  strcpy(ssidToChar, ssid.c_str());
  strcpy(passToChar, pass.c_str());

  _ssid = ssidToChar;
  _pass = passToChar;

  WiFi.begin(_ssid, _pass); //Connect to access point

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DEBUG_MSG("waiting to connect ot wifi");
    if (authFailed())
      break;
  }

  if (authSuccess())
  {
    DEBUG_MSG("Connected to: ");
    DEBUG_MSG(_ssid);
    DEBUG_MSG("IP address: ");
    DEBUG_MSG(WiFi.localIP().toString().c_str());
  }

  //Start UDP
  /*Serial.println("Starting UDP");
  udp.begin(_localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());*/
}

bool UDPcommunication::authSuccess()
{
  return WiFi.status() == WL_CONNECTED;
}

bool UDPcommunication::authFailed()
{
  return (WiFi.status() == WL_CONNECT_FAILED) || (WiFi.status() == WL_NO_SSID_AVAIL);
}

bool UDPcommunication::changingStatus()
{
  return WiFi.status() == WL_IDLE_STATUS;
}

bool UDPcommunication::disconnected()
{
  return WiFi.status() == WL_DISCONNECTED;
}

void UDPcommunication::stopListenForCredentials()
{
  //WiFi.mode(WIFI_AP_STA);
  udp->stop();
  WiFi.softAPdisconnect(true);
  APRunning = false;
  UDPRunning = false;
  delay(100);
}

char *UDPcommunication::listenForCredentials()
{
  if (APRunning && UDPRunning)
  {
    int cb = udp->parsePacket();
    if (cb)
    {
      int leng = udp->available();
      char *packetBuffer = new char[leng];
      //memset(packetBuffer, 0, sizeof(packetBuffer));
      udp->read(packetBuffer, leng);
      return packetBuffer;
    }
    else
    {
      return NULL;
    }
  }
  else
  {
    if (!APRunning)
    {
      // WiFi.mode(WIFI_AP);
      DEBUG_MSG("Setting soft-AP configuration ... ");
      DEBUG_MSG(WiFi.softAPConfig(serverIP, gatewayIP, subnetIP) ? "Ready" : "Failed!");

      DEBUG_MSG("Setting soft-AP ... ");
      bool result = WiFi.softAP("GreenHouse_setup_WiFi");
      DEBUG_MSG(result ? "Ready" : "Failed!");

      DEBUG_MSG("Soft-AP IP address: ");
      DEBUG_MSG(WiFi.softAPIP().toString().c_str());
      APRunning = result;
      delay(1000);
    }

    if (WiFi.softAPgetStationNum() != 0)
    {

      DEBUG_MSG("Starting UDP");
      udp->begin(_localPort);
      UDPRunning = true;
      DEBUG_MSG("Local port: ");
      DEBUG_MSG(String(udp->localPort()).c_str());
      DEBUG_MSG("Listenning for credentials packet");
      delay(1000);
    }
    else
    {
      delay(1000);
      DEBUG_MSG("Wait for clien");
    }

    return NULL;
  }
}

char *UDPcommunication::listenForHub()
{
  if (UDPRunning)
  {
    int cb = udp->parsePacket();
    if (!cb)
    {
      udp->beginPacket(brodcastIP, _localPort);
      String a = "gimme your ip bro";
      char b[a.length()];
      a.toCharArray(b, a.length() + 1);
      udp->write(b, a.length()); //Send one byte
      udp->endPacket();
      delay(100);
    }
    else
    {
      int leng = udp->available();
      char *packetBuffer = new char[leng];
      //memset(packetBuffer, 0, sizeof(packetBuffer));
      udp->read(packetBuffer, leng);
      //Serial.println(udp.remoteIP());
      String testtext = String(packetBuffer);
      if (testtext[0] == 'o')
      {
        String ip = udp->remoteIP().toString();
        char *copy = new char[ip.length() + 1];
        strcpy(copy, ip.c_str());
        //hubIP = hubIP.fromString(ip.c_str());
        hubIP.fromString(ip.c_str());
        return copy;
      }
      else
      {
        return NULL;
      }
    }
  }
  else
  {
    DEBUG_MSG("Starting UDP");
    udp->begin(_publicPort);
    DEBUG_MSG("Local port: ");
    DEBUG_MSG(String(udp->localPort()).c_str());
    DEBUG_MSG("Address: ");
    DEBUG_MSG(WiFi.localIP().toString().c_str());
    UDPRunning = true;
    return NULL;
  }

  return NULL;
}

bool UDPcommunication::haveHub()
{
  return hubIP.isSet();
}

void UDPcommunication::check()
{
  int cb = udp->parsePacket();
  char packetBuffer[9];
  if (!cb)
  {
    //If serial data is recived send it to UDP
    if (Serial.available() > 0)
    {
      udp->beginPacket(hubIP, _publicPort); //Send Data to Master unit
      //Send UDP requests are to port 2000

      //char a[1];
      String a = Serial.readString();
      Serial.println(a);
      //a[0]=char(test[0]); //Serial Byte Read
      char b[a.length()];
      a.toCharArray(b, a.length());
      udp->write(b, a.length()); //Send one byte to ESP8266
      udp->endPacket();
    }
  }
  else
  {
    // We've received a UDP packet, send it to serial
    memset(packetBuffer, 0, sizeof(packetBuffer));
    udp->read(packetBuffer, 10); // read the packet into the buffer, we are reading only one byte
    Serial.println(packetBuffer);
  }
}