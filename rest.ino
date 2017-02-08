// Very basic Spiffs example, writing 10 strings to SPIFFS filesystem, and then read them back
// For SPIFFS doc see : https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md
// Compiled in Arduino 1.6.7. Runs OK on Wemos D1 ESP8266 board.

#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>  // more info: https://github.com/bblanchon/ArduinoJson



#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);
boolean isServer = true;


/*
   Metoda vypise JSON, ktory sa posielan na web server
   @param int water_detected,int battery_voltage,String SN
*/
bool sendJsonData(int water_detected, int battery_voltage, String SN) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();  //schovat do metody sendjsondata
  root["water_detected"] = water_detected;
  root["battery_voltage"] = battery_voltage;
  root["S/N"] = SN;
  root.printTo(Serial);  // bude potrebne zmenit na clienta
  // root.prettyPrintTo(Serial);  //pekny vypis na terminal
}

/*
 * Metoda ulozi SN zariadenia ktory je vygenerovany zo serveru 
 */
bool saveSN(String data) {
  SPIFFS.begin();
  File f = SPIFFS.open("/sn.cfg", "w");
  if (!f) {
    Serial.println("sn.cfg open failed");
    return false;
  }
  Serial.println("====== Writing to sn.cfg file =========");
  Serial.println(data);
  f.println(data);
  f.close();
  SPIFFS.end();
  Serial.println("File saved");
  return true;
  }


/*
   Metoda vrati seriove cislo SN, ktore je generovane 
*/
String getSN() {
  String sn = "-1";
  File f = SPIFFS.open("/sn.cfg", "r");
  if (!f) {
    Serial.println("sn.cfg can not open");
  }
  sn = f.readStringUntil('\n');
  f.close();
  if (sn == "-1") {
    Serial.println("sn filne can not read");
    return "can not read SN number";
  }
  
  Serial.println(sn);
  return sn;
}



/*
  Metoda vrati hodnotu struingu
  ak vrati 1 a ine - ESP sa prepne do APmodu
  ak vrati 0 ESP sa prepne do modu wifi clinet
*/
String getMode() {
  String vystup = "";
  File f = SPIFFS.open("/config.txt", "r");
  if (!f) {
    Serial.println("file open failed");
  }

  for (int i = 1; i <= 1; i++) {
    vystup = f.readStringUntil('\n');
  }
  f.close();
  return vystup;
}
/*
  Metoda vrati hodnotu struingu, ktora je heslo acces  pointu
*/
String getHesloAP() {
  String vystup = "";
  File f = SPIFFS.open("/config.txt", "r");
  if (!f) {
    Serial.println("file open failed");
  }

  for (int i = 1; i <= 2; i++) {
    vystup = f.readStringUntil('\n');
  }
  f.close();
  return vystup;
}

/*
  Metoda vrati hodnotu struingu, ktora obsahuje nazov Access pointu
*/
String getNazovAP() {
  String vystup = "";
  File f = SPIFFS.open("/config.txt", "r");
  if (!f) {
    Serial.println("file open failed");
  }

  for (int i = 1; i <= 3; i++) {
    vystup = f.readStringUntil('\n');
  }
  f.close();
  return vystup;
}
/*
   Metoda vrati interval po akom sa ma restartnut ESP
*/
String getIntervalRestartu() {
  String interval = "-1";
  File f = SPIFFS.open("/interval.cfg", "r");
  if (!f) {
    Serial.println("interval.cfg can not open");
  }
  interval = f.readStringUntil('\n');
  f.close();
  if (interval == "-1") {
    Serial.println("Interval can not read");
  }
  return interval;
}

String getRiadok(int n) {
  String vystup = "";
  File f = SPIFFS.open("/config.txt", "r");
  if (!f) {
    Serial.println("file open failed");
  }

  for (int i = 1; i <= n; i++) {
    vystup = f.readStringUntil('\n');
  }
  f.close();
  return vystup;
}


/*
   Metoda vrati HTML request
*/
// prepare a web page to be send to a client (web browser)
String prepareHtmlPage(String i)
{
  String htmlPage =
    String("HTTP/1.1 200 OK\r\n") +
    "Content-Type: text/html\r\n" +
    "Connection: close\r\n" +  // the connection will be closed after completion of the response
    //"Interval: " + String(i) + "\r\n" + // refresh the page automatically every 5 sec
    "\r\n" +
    "<!DOCTYPE HTML>" +
    "<html>" +
    "Vystup :  " + String(i) +
    "</html>" +
    "\r\n";
  return htmlPage;
}
/*
 * ulozi interval restartu do konfiguracneho suboru
 */
void setIntervalRestartu(int interval) {

  SPIFFS.begin();
  File fw = SPIFFS.open("/interval.cfg", "w");
  if (!fw) {
    Serial.println("interval.cfg can not be open");
  }
  fw.print(interval);
  fw.close();
  SPIFFS.end();
  Serial.println("interval restartu bol zmeneny");

}




void setup() {
  delay(1000);
  Serial.begin(115200);
  SPIFFS.begin();
  /*
    //nastavi Nazov AP
    String stringNazovAP = getNazovAP();
    char charNazovAP[sizeof(stringNazovAP)];
    stringNazovAP.toCharArray(charNazovAP, sizeof(charNazovAP));
    const char *ssid = charNazovAP;
    //nastavi heslo AP
    String stringPassword = getHesloAP();
    char charPassword[sizeof(stringPassword)];
    stringPassword.toCharArray(charPassword, sizeof(charPassword));
    char *password = charPassword;
    Serial.print("heslo:");
    Serial.println(password);

    rest.function("interval", zmenInterval);
  */
  if (getMode() != 0) {
    isServer = true;
    /*
      //nastavi Nazov Serveru
      String stringNazovAP = getNazovAP();
      char charNazovAP[sizeof(stringNazovAP)];
      stringNazovAP.toCharArray(charNazovAP, sizeof(charNazovAP));
      //const char *ssid = charNazovAP;
      const char *ssid = "sladkovicova";
      Serial.print(ssid);
      //nastavi heslo AP
      String stringPassword = getHesloAP();
      char charPassword[sizeof(stringPassword)];
      stringPassword.toCharArray(charPassword, sizeof(charPassword));
      //char *password = charPassword;
      char *password = "sladkovicova";
      Serial.print("heslo:");
      Serial.println(password);

      WiFi.begin(charNazovAP, charPassword);
    */
    //WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP", "123123123");
    //WiFi.begin("ESP86", "123123123");
    while (WiFi.status() != 1)
    {
      delay(500);
      Serial.print(".");
      Serial.print(WiFi.status());
    }

    server.begin();
    Serial.println("\nHTTP server started");

  }
  else {
    isServer = false;
    Serial.println("Startuje sa wifi client mode");
  }


  //testovacie vypisy
  Serial.println(getHesloAP());
  Serial.println(getNazovAP());
  Serial.println(getIntervalRestartu());
  sendJsonData( 0, 35, "sdasdsad54645g6rfdgdf54we6DSFD5");
  Serial.println(getSN());

  SPIFFS.end();
}



void loop() {
  if (isServer == true) {
    WiFiClient client = server.available();
    // wait for a client (web browser) to connect
    if (client)
    {
      Serial.println("\n[Client connected]");
      while (client.connected())
      {
        // read line by line what the client (web browser) is requesting
        if (client.available())
        {
          String req = client.readStringUntil('\r');
          if (req.length() < 1) {
            return;
          }
          // Serial.println(req);

          Serial.println("koniec vypisu ");
          int val;
          if (req.indexOf("/interval/0") != -1) {
            client.println(prepareHtmlPage("0"));
            setIntervalRestartu(0);
            break;
          }
          if (req.indexOf("/interval/1") != -1) {
            client.println(prepareHtmlPage("1"));
            setIntervalRestartu(1);
            break;
          }
          if (req.indexOf("/interval/2") != -1) {
            client.println(prepareHtmlPage("2"));
            setIntervalRestartu(2);
            break;
          }
          if (req.indexOf("/interval/3") != -1) {
            client.println(prepareHtmlPage("3"));
            setIntervalRestartu(3);
            break;
          }
          if (req.indexOf("/interval/4") != -1) {
            client.println(prepareHtmlPage("4"));
            setIntervalRestartu(4);
            break;
          }
          if (req.indexOf("/interval/5") != -1) {
            client.println(prepareHtmlPage("5"));
            setIntervalRestartu(5);
            Serial.println("5");

            break;
          }
          if (req.indexOf("/interval/6") != -1) {
            client.println(prepareHtmlPage("6"));
            setIntervalRestartu(6);
            break;
          }
          if (req.indexOf("/interval/7") != -1) {
            client.println(prepareHtmlPage("7"));
            setIntervalRestartu(7);
            break;
          }
          
          int index = req.indexOf("/sn/");
          if (index !=-1) {
            // tu bol problem ze cely string bol: GET /sn/ahoj%20svet HTTP/1.1 bolo treba parsovat 
            index+=4;
            String pom = req.substring(index,(req.length()-9));
            saveSN(pom);
            client.println(prepareHtmlPage(pom));
            break;
          }

          else {
            client.println(prepareHtmlPage("Bad request"));
            Serial.print(req);
            //poterbne osetrit chybu!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            break;
          }




        }
      }
      delay(1); // give the web browser time to receive the data

      // close the connection:
      client.stop();
      Serial.println("[Client disonnected]");
    }
  }
}
/*
   Momentalne sa tato metoda nepouziva
   neni potrebne lebo sa volaju get metody
*/
int zmenInterval(String command) {
  SPIFFS.begin();
  // Get state from command
  int interval = command.toInt();
  setIntervalRestartu(interval);


  return 1;

}
