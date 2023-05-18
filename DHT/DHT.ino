/*
// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

Blynk virtual pin
V0 - temp
V1 - humid

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

// ตั้งค่า
char ssid[] = "G6PD_2.4G";
char pass[] = "570610193";
#define LINE_TOKEN "7m68381D2LS8ByeflY4rVEf9pPEXMXllsuFRNGBTFfG"
#define TEMP_H 35
#define HUMID_H 80

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN D7

#define DHTTYPE DHT11  // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
bool tempNotiState, humidNotiState;

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

    display.clearDisplay();
    display.setTextSize(2);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);              // Start at top-left corner
    display.println(F("DHT sensor"));
    display.setCursor(0, 25);
    display.println("T : " + String(t, 1) + " C");
    display.println("H : " + String(h, 1) + " %");
    display.display();

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.println(F("°C"));

    // แจ้งเตือนอุณหภูมิผ่านไลน์
    if (t >= TEMP_H) {
      if (!tempNotiState) {
        LINE.notify("อุณหภูมิเกินกำหนด อุณหภูมิขณะนี้ " + String(t) + " C");
        tempNotiState = 1;
      }
    } else if (tempNotiState) {
      tempNotiState = 0;
    }

    // แจ้งเตือนความชื้นผ่านไลน์
    if (h >= HUMID_H) {
      if (!humidNotiState) {
        LINE.notify("ความชื้นเกินกำหนด ความชื้นขณะนี้ " + String(h) + " %");
        humidNotiState = 1;
      }
    } else if (humidNotiState) {
      humidNotiState = 0;
    }

    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
}
