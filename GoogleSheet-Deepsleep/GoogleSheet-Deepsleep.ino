//-----------------------------------------------
// Author: Luca Großhennig
// Email: luca.grosshennig@gmx.de
// Publish date: 28-JUL-2020
// Description: This code for demonstration send data from ESP8266 into Google Spreadsheet, deppsleeps
//              and saves data localy if no wifi is available
// update ssid, password and GAS_ID
//-----------------------------------------------

#include <ESP8266WiFi.h>
#include "SparkFunBME280.h"
#include <RtcDS3231.h> //RTC library
#include <WiFiClientSecure.h>
#include <SPI.h>

#include "LittleFS.h"

#define durationSleep 900  * 1e6 // in seconds  * 1e6

BME280 BME;
RtcDS3231<TwoWire> RTC(Wire);
WiFiClientSecure httpsClient;
File myfile;

const char* ssid = "***"; // Wlan name here
const char* password = "***"; // Wlan password here
const char* host = "script.google.com";
const int httpsPort = 443;

String filename = "/data.csv";
String GAS_ID = "***"; // Google script id here

int retry = 0;
int retry_number = 15;

const int sensor_pin = A0;

typedef struct {
  float t;
  int h;
  int p;
  int r;
  String time_string;
} measurement;

measurement data[1] = {0, 0, 0, 0, "0"};


void getTime() {
  RtcDateTime currentTime = RTC.GetDateTime();    //get the time from the RTC

  char str[20];   //declare a string as an array of chars
  sprintf(str, "%d/%d/%d-%d:%d",     //%d allows to print an integer to the string
          currentTime.Year(),   //get year method
          currentTime.Month(),  //get month method
          currentTime.Day(),    //get day method
          currentTime.Hour(),   //get hour method
          currentTime.Minute() //get minute method
         );
  data[0].time_string = str;
}

void getSensor() {
  data[0].h = BME.readFloatHumidity();
  data[0].t = BME.readTempC();
  data[0].p = BME.readFloatPressure() / 100;
  data[0].r = analogRead(sensor_pin);
  if (isnan(data[0].h) || isnan(data[0].t) || isnan(data[0].p)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
}
void setup() {
  WiFi.mode( WIFI_OFF );   // for lower power consumpton
  WiFi.forceSleepBegin();
  delay( 1 );
  
  Serial.begin(9600); //Serial
  pinMode(sensor_pin, INPUT);
  delay(500);
  Serial.println();
  SPI.begin();
  RTC.Begin();
  setup_bme();
  retry = 0;
  getSensor();
  getTime();
  bool wifi = setup_wifi();
  retry = 0;
  bool InitOK = InitalizeFileSystem();
  retry = 0;
  if (wifi) {
    if (InitOK) {
      sendSavedData();
      retry = 0;
    }else{
      Serial.println("init not OK");
      }
    retry = 0;
    sendData(data);
    retry = 0;
  } else {
    if (InitOK) {
      saveData(data[0]);
      retry = 0;
    }
  }
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  go_sleep();
}

void loop() {
}

void go_sleep(){
  WiFi.disconnect( true );
  delay( 1 );
  ESP.deepSleep(durationSleep , WAKE_RF_DISABLED );
  }

void setup_bme() {
  Serial.println();
  Serial.print("Connecting to BME280 ");
  BME.setI2CAddress(0x76);
  while ((!BME.beginI2C()) && retry < retry_number) {
    BME.setI2CAddress(0x76);
    Serial.print(".");
    retry++;
    delay(500);
  }
  Serial.println();
  if (retry == retry_number) {
    Serial.println("Conection to BME280 failed");
    go_sleep();
  }
  retry = 0;
  Serial.println("Connected to BME280");
}

boolean setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
 
  WiFi.forceSleepWake(); // for lower power consumption
  delay( 1 );
  WiFi.persistent( false );
  WiFi.mode( WIFI_STA );

  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && retry < retry_number) {
    Serial.print(".");
    delay(750);
    retry++;      // Wenn nach 10 Versuchen nicht mit WiFi verbunden werden kann, deep-sleep
  }
  if (retry == retry_number ) {
    Serial.println();
    Serial.println("Kann nicht mit WiFi verbunden werden, speichere Daten lokal");
    Serial.println();
    return (false);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  return (true);
}

void sendSavedData() {// does not work now. Just prints it line by line
  httpsClient.setInsecure();
  Serial.print("Connecting to Google Sheet ");
  while ((!httpsClient.connect(host, httpsPort)) && (retry < retry_number)) {
    delay(100);
    Serial.print(".");
    retry++;
  }
  Serial.println();
  if (retry == retry_number) {
    Serial.println("Connection failed");
    saveData(data[0]);
    go_sleep();
  } else {
    Serial.println("Connected to Server");
  }

  Serial.println();
  if (LittleFS.exists(filename)) {
    Serial.println("sending saved data");
    Serial.println("--------------------------------------------");

    myfile = LittleFS.open(filename, "r");

    String content;
    while (myfile.position() < myfile.size())
    {
      content = myfile.readStringUntil('\n');  // split content to data
      String da[5]; // elements to split the string into
      int r = 0, t = 0;
      for (int i = 0; i < content.length(); i++)
      {
        if (content.charAt(i) == ';')
        {
          da[t] = content.substring(r, i);
          r = (i + 1);
          t++;
        }
      }// splitting string
      //  da[1]   temp
      //  da[2]   hum
      //  da[3]   pres
      //  da[4]   rainfall
      //  da[5]   time

      String url = "https://" + String(host) + "/macros/s/" + GAS_ID + "/exec?temp=" + da[0] + "&hum=" + da[1] + "&press=" + da[2] + "&rain=" + da[3] + "&time=" + da[4];
      httpsClient.print(String("GET ") + url +
                        " HTTP/1.1\r\n" +
                        "Host: " + host +
                        "\r\n" + "Connection: Keep-Alive\r\n\r\n");
      Serial.println(url);
      delay(750);
    }
    httpsClient.print(String("GET ") + "https://" + String(host) + "/" +
                      " HTTP/1.1\r\n" +
                      "Host: " + host +
                      "\r\n" + "Connection: close\r\n\r\n");
    myfile.close();
    Serial.println("--------------------------------------------");
    LittleFS.remove(filename);
  }
  else {
    return;
  }
}

void saveData(measurement data) {
  Serial.println();
  Serial.println("Saving Data localy");
  if (!LittleFS.exists(filename)) {
    myfile = LittleFS.open(filename, "w");
  } else {
    myfile = LittleFS.open(filename, "a");
  }
  if (!myfile) {
    Serial.println("Fehler beim schreiben der Datei");
  } else {
    Serial.print("Save one line:");
    Serial.println(String(data.t) + ";" + String(data.h) + ";" + String(data.p) + ";" + String(data.r) + ";" + data.time_string + ";");
    myfile.println(String(data.t) + ";" + String(data.h) + ";" + String(data.p) + ";" + String(data.r) + ";" + data.time_string + ";");
    myfile.close();
  }
}

// Function for Send data into Google Spreadsheet
void sendData(measurement data[]) {
  httpsClient.setInsecure();

  String url = "https://" + String(host) + "/macros/s/" + GAS_ID + "/exec?temp=" + String(data[0].t) + "&hum=" + String(data[0].h) + "&press=" + String(data[0].p) + "&rain=" + String(data[0].r) + "&time=" + data[0].time_string;
  Serial.println(url);
  Serial.print("Connecting to Google Sheet ");
  while ((!httpsClient.connect(host, httpsPort)) && (retry < retry_number)) {
    delay(100);
    Serial.print(".");
    retry++;
  }
  Serial.println();
  if (retry == retry_number) {
    Serial.println("Connection failed");
    saveData(data[0]);
    go_sleep();
  } else {
    Serial.println("Connected to Server");
  }
  httpsClient.print(String("GET ") + url +
                    " HTTP/1.1\r\n" +
                    "Host: " + host +
                    "\r\n" + "Connection: close\r\n\r\n");

  Serial.println("Data sent");
}

boolean InitalizeFileSystem() {
  Serial.print("Initializing LittleFS ");
  bool initok = false;
  initok = LittleFS.begin();
  while ((!initok) && retry < retry_number) {
    Serial.print(".");
    LittleFS.format();
    initok = LittleFS.begin();
    retry++;
  }
  Serial.println();
  if (initok) {
    Serial.println("LittleFS succesessfuly initialized");
  } else {
    Serial.println("LittleFS error");
  }
  return initok;
}