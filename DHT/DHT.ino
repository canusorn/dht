/*
 REQUIRES the following Arduino libraries:
 - DHT Sensor Library ติดตึั้งจาก library manager
 - Adafruit Unified Sensor Lib ติดตึั้งจาก library manager
 - Adafruit_SSD1306 ติดตึั้งจาก library manager
 - Blynk  ติดตึั้งจาก library manager


Blynk virtual pin
V0 - temp
V1 - humid
V2 - smoke

การต่อวงจร
DHT22 -> ESP8266
  +  ->  3v
  -  ->  gnd
data ->  D7

oled -> ESP8266
SCK ->  D1
SCL ->  D2
VCC ->  3V
GND ->  GND

mq2 ->  ESP8266
A0  ->  A0
VCC ->  3v
GND ->  GND

*/
#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6PgBGBfDj"
#define BLYNK_TEMPLATE_NAME "DHT"
#define BLYNK_AUTH_TOKEN "Ri68VcXPAOPX-EGSj42IszrWZQPBOgxd"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TridentTD_LineNotify.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

// ตั้งค่า
char ssid[] = "G6PD_2.4G";
char pass[] = "570610193";
#define LINE_TOKEN "7m68381D2LS8ByeflY4rVEf9pPEXMXllsuFRNGBTFfG"
String GSHEET_API = "https://script.google.com/macros/s/AKfycbwnw55UmfUBB3M9ZTPWwmPjoZ1dQ_glG11TgRI94RA980hmTni7dAbR6R782pO46AxS/exec";

// ตั้งค่าแจ้งเตือนเมื่อมีค่ามากกว่า
#define TEMP_H 35
#define HUMID_H 80
#define SMOKE_H 50

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN D7

#define DHTTYPE DHT11  // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
bool tempNotiState, humidNotiState, smokeNotiState;

void setup() {

  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextSize(2);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(0, 0);              // Start at top-left corner
  display.println(F("DHT sensor"));
  display.display();

  dht.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // กำหนด Line Token
  LINE.setToken(LINE_TOKEN);
}

void loop() {
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 5000) {
    previousMillis = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int smoke = analogRead(A0);
    smoke = smoke * 100 / 1023;

    display.clearDisplay();
    display.setTextSize(2);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);              // Start at top-left corner
    // display.println(F("DHT sensor"));
    // display.setTextSize(1);
    // display.setCursor(0, 25);
    display.println("T : " + String(t, 1) + " C");
    display.println("H : " + String(h, 1) + " %");
    display.println("S : " + String(smoke) + " %");
    display.display();

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C  Smoke: "));
    Serial.print(smoke);
    Serial.println(F("%"));

    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
    Blynk.virtualWrite(V2, smoke);

    // แจ้งเตือนอุณหภูมิผ่านไลน์
    if (t >= TEMP_H) {
      if (!tempNotiState) {
        LINE.notify("อุณหภูมิเกินกำหนด อุณหภูมิขณะนี้ " + String(t) + " C");
        tempNotiState = 1;
        Blynk.setProperty(V0, "color", "red");
      }
    } else if (tempNotiState) {
      tempNotiState = 0;
      Blynk.setProperty(V0, "color", "#252757");
    }

    // แจ้งเตือนความชื้นผ่านไลน์
    if (h >= HUMID_H) {
      if (!humidNotiState) {
        LINE.notify("ความชื้นเกินกำหนด ความชื้นขณะนี้ " + String(h) + " %");
        humidNotiState = 1;
        Blynk.setProperty(V1, "color", "red");
      }
    } else if (humidNotiState) {
      humidNotiState = 0;
      Blynk.setProperty(V1, "color", "#252757");
    }

    // แจ้งเตือนควันผ่านไลน์
    if (smoke >= SMOKE_H) {
      if (!smokeNotiState) {
        LINE.notify("ควันเกินกำหนด ควันขณะนี้ " + String(smoke) + " %");
        smokeNotiState = 1;
        Blynk.setProperty(V2, "color", "red");
      }
    } else if (smokeNotiState) {
      smokeNotiState = 0;
      Blynk.setProperty(V2, "color", "#252757");
    }

    sentToSheet(t, h, smoke);
  }
}


void sentToSheet(float t, float h, int smoke) {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

  client->setInsecure();

  HTTPClient https;

  String url = GSHEET_API;
  url += "?temperature=";
  url += String(t, 1);
  url += "&humidity=";
  url += String(h, 1);
  url += "&smoke=";
  url += String(smoke);

  Serial.print("[HTTPS] begin...\n");
  if (https.begin(*client, url)) {  // HTTPS

    Serial.print("[HTTPS] GET...\n");
    // start connection and send HTTP header
    int httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
}