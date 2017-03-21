// Very basic Spiffs example, writing 10 strings to SPIFFS filesystem, and then read them back
// For SPIFFS doc see : https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md
// Compiled in Arduino 1.6.7. Runs OK on Wemos D1 ESP8266 board.

#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ArduinoJson.h>  // more info: https://github.com/bblanchon/ArduinoJson



#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);
boolean isServer = true;
bool isSnConfigurated;
const char* ssid = "Internet";
const char* password = "3krakousek3";
//const char* ssid = "Meizu";
//const char* password = "123123123";
const char* host = "85.93.125.205";
bool succes = false;
int pocetzlych = 0;
int failedPosts = 0;


/*
   Metoda vypise JSON, ktory sa posielan na web server
   @param int water_detected,int battery_voltage,String SN
*/

void sendJsonData(String DeviceID, int BatteryLife, int state) {

  WiFiClient client;
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["DeviceID"] = DeviceID;
  json["BatteryLife"] = BatteryLife;
  json["State"] = state;
  json.printTo(Serial);
  Serial.println();


  Serial.printf("\n[Connecting to %s ... ", host);
  if (client.connect(host, 8126))
  {
    Serial.println("connected]");
    Serial.println("[Sending a request]");

    client.println("POST /newdata HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("Content-Type: application/json");
    int length = json.measureLength();
    client.print("Content-Length:"); client.println(length);
    // End of headers
    client.println();
    String out;
    json.printTo(out);
    client.println(out);

    Serial.println("[Response:]");
    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        if (line.indexOf("HTTP/1.1 200 OK") != -1) {
          succes = true;
        }
      }
    }
    if (succes == true) {
      Serial.println("Data boli uspesne odoslane");
    }
    else {
      Serial.println("Data sa nepodarilo odoslat ide restart");
      //v pripade, ze sa nepodairlo odoslat validne data nastavi ESP na server mode a restartme
      //setmode(); nasvaime tak aby bol server
      ESP.deepSleep(500);

    }
    client.stop();
    Serial.println("\n[Disconnected]");
  }
  else
  {
    Serial.println("connection failed!]");
    client.stop();
  }
}




void getStatus() {
  int setup = -1;
  String DeviceID = getData("SN");
  //String DeviceID = DeviceIdNoParse.substring(0,DeviceIdNoParse.length()-1);
  Serial.println(DeviceID);
  WiFiClient client;
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["DeviceID"] = DeviceID;
  Serial.println();
  Serial.println("vypis JSON");
  json.printTo(Serial);
  Serial.println();



  failedPosts = 0;
  pocetzlych = 0;
  Serial.println("Startuje sa wifi client mode");
  Serial.printf("Connecting to %s ", ssid);
  Serial.printf("Heslo: %s", password);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && (pocetzlych < 40))
  {
    delay(500);
    Serial.print(".");
    //Serial.print(WiFi.status());
    pocetzlych++;
  }
  if (pocetzlych > 39) {
    Serial.println("Connection filed \nConfig you AP\nStarting AP mode");
    saveData("mode", "1");

    //ESP.deepSleep(1000);
  }
  else {
    while ((setup == -1) && (failedPosts < 16)) {
      failedPosts++;
      Serial.printf("\n[Connecting to %s ... ", host);
      if (client.connect(host, 8126))
      {

        Serial.println("connected]");

        Serial.println("[Sending a request]");

        client.println("POST /api/getsetup HTTP/1.1");
        client.print("Host: ");
        client.println(host);
        client.println("Content-Type: application/json");
        int length = json.measureLength();
        client.print("Content-Length:"); client.println(length);
        // End of headers
        client.println();
        String out;
        json.printTo(out);
        client.println(out);

        delay(1000);
        Serial.println("[Response:]");
        int indexOfLine = 0;
        /*
          unsigned long timeoutStart = millis();
          const int kNetworkTimeout = 30 * 1000;
          // Number of milliseconds to wait if no data is available before trying again
          const int kNetworkDelay = 1000;
          char c;
          while ( (client.connected() || client.available()) &&
                ((millis() - timeoutStart) < kNetworkTimeout) )
          {
          if (client.available())
          {
            c =client.read();
            // Print out this character
            Serial.print(c);

            //bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          }
          else
          {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
          }
          }
        */

        while (client.connected())
        {
          if (client.available())
          {
            String line = client.readStringUntil('\n');
            //Serial.println(line);
            indexOfLine++;

            if ((line.indexOf("0") != -1) && indexOfLine > 7) {
              setup = 0;
            }
            else {
              if ((line.indexOf("1") != -1) && indexOfLine > 7) {
                setup = 1;
              }
              else {
                setup = -1;
              }
            }
          }
        }
        Serial.printf("Setup: %d\n", setup);
        if (setup == 1) {
          client.stop();
          Serial.println("\n[Disconnected]");
          saveData("mode", "1");
        }
        if (setup == 0) {
          saveData("mode", "0");
        }


        client.stop();
        Serial.println("\n[Disconnected]");
        //
      }
      else
      {
        Serial.println("Can not connect to server!]");
        client.stop();
        break;
      }
    }
    if (setup == -1) {
      Serial.printf("Wrong sensor ID - please configurate your device\n");
    }
  }
}

/*
   Metoda zisti ci je volozene SN
*/
bool isDataConfigrated(String path) {
  SPIFFS.begin();
  bool result = SPIFFS.exists("/" + path + ".cfg");
  SPIFFS.end();
  return result;
}


/*
   Metoda ulozi Data do suboru
   Nazvy suborov: SN, nazovAP, passwordAP, ssidWifi. passwordWifi
*/
bool saveData(String path, String data) {
  SPIFFS.begin();
  File f = SPIFFS.open("/" + path + ".cfg", "w");
  if (!f) {
    Serial.println("sn.cfg open failed");
    return false;
  }
  Serial.println("====== Writing to " + path + ".cfg file =========");
  Serial.println(data);
  f.println(data);
  f.close();
  SPIFFS.end();
  Serial.println("========= File saved =========");
  Serial.println();
  Serial.println();
  return true;
}


/*
   Metoda vrati seriove cislo SN, ktore je generovane
*/
String getData(String path) {
  String snNoParsed = "-1";
  SPIFFS.begin();
  File f = SPIFFS.open("/" + path + ".cfg", "r");
  if (!f) {
    Serial.println("sn.cfg can not open");
  }
  snNoParsed = f.readStringUntil('\n');
  String sn = snNoParsed.substring(0, snNoParsed.length() - 1);
  f.close();
  SPIFFS.end();
  if (sn == "-1") {
    Serial.println("sn filne can not read");
    return "can not read " + path + " number";
  }
  return sn;
}


/*
  Metoda vrati hodnotu struingu
  ak vrati 3 a ine - ESP nebolo nikdy konfigurovane
  ak vrati 1 a ine - ESP sa prepne do APmodu
  ak vrati 0 ESP sa prepne do modu wifi clinet
*/
int getMode() {
  String vystup = "";
  SPIFFS.begin();
  File f = SPIFFS.open("/mode.cfg", "r");
  if (!f) {
    Serial.println("file open failed");
  }

  for (int i = 1; i <= 1; i++) {
    vystup = f.readStringUntil('\n');
  }
  f.close();
  SPIFFS.end();
  return vystup.toInt();
}


/*
   Metoda vrati interval po akom sa ma restartnut ESP
*/
String getIntervalRestartu() {
  String interval = "-1";
  SPIFFS.begin();
  File f = SPIFFS.open("/interval.cfg", "r");
  if (!f) {
    Serial.println("interval.cfg can not open");
  }
  interval = f.readStringUntil('\n');
  f.close();
  SPIFFS.end();
  if (interval == "-1") {
    Serial.println("Interval can not read");
  }
  return interval;
}

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

String getRiadok(int n) {
  String vystup = "";
  SPIFFS.begin();
  File f = SPIFFS.open("/config.txt", "r");
  if (!f) {
    Serial.println("file open failed");
  }

  for (int i = 1; i <= n; i++) {
    vystup = f.readStringUntil('\r');
  }
  f.close();
  SPIFFS.end();
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
   ulozi interval restartu do konfiguracneho suboru
*/


void startAP() {
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  //nastavi Nazov Serveru
  String stringNazovAP = getData("nazovAP");
  char charNazovAP[stringNazovAP.length()];
  Serial.println(stringNazovAP);
  stringNazovAP.toCharArray(charNazovAP, stringNazovAP.length() + 1);
  const char *ssid = charNazovAP;
  Serial.print("SSID: ");
  Serial.println(ssid);
  //nastavi heslo AP
  String stringPassword = getData("passwordAP");
  char charPassword[stringPassword.length()];
  stringPassword.toCharArray(charPassword, stringPassword.length() + 1);
  char *password = charPassword;
  //char *password = "sladkovicova";
  Serial.print("heslo:");
  Serial.println(password);
  //WiFi.softAPdisconnect();


  WiFi.softAP(charNazovAP, charPassword);


  server.begin();
  Serial.println("\nHTTP server started");
}



//Nazvy suborov: SN, nazovAP, passwordAP, ssidWifi. passwordWifi
void setup() {
  delay(1000);
  Serial.begin(115200);
  SPIFFS.begin();
  getStatus();
  //saveData("mode", "1");  //0 - client
  /*
    saveData("nazovAP", "ESP");
    String naz = getData("nazovAP");
    Serial.print("Nazov: ");
    Serial.println(naz);
    saveData("passwordAP", "123123123");
    String pass = getData("passwordAP");
    Serial.print("pass: ");
    Serial.println(pass);

    //saveData("SN", "$2y$10$q0m40L8nRMr.bPoBkk4p7OptBvfa2YSRtTv5uetJ430G/7WYzEdHe");
    Serial.print("SN: ");
    Serial.println(getData("SN"));
  */
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
  Serial.println();
  Serial.print("Mode: ");
  Serial.println(getMode());


  if (getMode() != 0) {
    isServer = true;
    startAP();
  }
  else {
    isServer = false;

    /*
      pocetzlych = 0;
      Serial.println("Startuje sa wifi client mode");
      Serial.printf("Connecting to %s ", ssid);
      Serial.printf("Heslo: %s", password);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      while ((WiFi.status() != WL_CONNECTED) && (pocetzlych < 40))
      {
      delay(500);
      Serial.print(".");
      Serial.print(WiFi.status());
      pocetzlych++;
      }
      if (pocetzlych > 39) {
      Serial.println("Connection filed \nStarting AP mode");
      saveData("mode", "1");
      ESP.deepSleep(1000);
      }
      else {*/
    Serial.println("Connected to Wifi \n Sending data");
    sendJsonData(getData("SN"), 93, 1);
    ESP.deepSleep(1000);//tu sa vlozi interval spanku
  }





  //testovacie vypisy
  Serial.print("Interval restartu: ");
  Serial.println(getIntervalRestartu());

  /*isSnConfigurated = isDataConfigrated("SN");
    Serial.print("Su data configurovane: ");
  */

  SPIFFS.end();
}



void loop() {
  if (isServer == true) {
    WiFiClient client = server.available();
    // wait for a client (web browser) to connect
    if (client)
    {
      int indexOfLine = 0;
      //Serial.println("\n[Client connected]");
      while (client.connected())
      {
        //Serial.println("Client Conected()");
        // read line by line what the client (web browser) is requesting
        if (client.available()) //if
        {
          String req = client.readStringUntil('\n');
          indexOfLine++;
          if (req.length() < 1) {
            return;
          }
          //Serial.println(req);
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

          if (indexOfLine > 9 && indexOfLine < 11) {
            Serial.println(req);
            Serial.println("Mame JSONN!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            makeJson(req);
            client.println("HTTP/1.1 200 OK");
            //client.println("HTTP/1.1 500 Internal server error");
            client.stop();
          }

          /*
                    else {
                      client.println(prepareHtmlPage("Bad request"));
                      Serial.print(req);
                      //poterbne osetrit chybu!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                      break;
                    }*/




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

void makeJson(String json) {
  /*
    char buffer[json.length()] ;
    json.toCharArray(buffer, json.length());
    Serial.println(buffer);
  */

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
  Serial.print("Vypis stringu:");
  Serial.println(json);
  root.printTo(Serial);
  Serial.println();

  
  const char* SN = root["SN"];
  const char* ssidWifi = root["ssidWifi"];
  const char* passwordWifi = root["passwordWifi"];
  const char* passwordAP = root["passwordAP"];
  const char* interval = root["interval"];
  Serial.print("Moj vypis SN:");
  Serial.println(SN);
  Serial.print("Moj vypis ssidWifi:");
  Serial.println(ssidWifi);
  Serial.print("Moj vypis passwordWifi:");
  Serial.println(passwordWifi);
  Serial.print("Moj vypis passwordAP:");
  Serial.println(passwordAP);
  Serial.print("Moj vypis interval:");
  Serial.println(interval);
}







