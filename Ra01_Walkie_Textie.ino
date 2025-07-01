#include <Adafruit_SSD1306.h>
#include <C:\Users\ardub\OneDrive\Belgeler\Arduino\libraries\Adafruit_GFX_Library\Fonts\Orbitron_Bold_14.h>
#include <C:\Users\ardub\OneDrive\Belgeler\Arduino\libraries\Adafruit_GFX_Library\Fonts\Orbitron_Bold_9.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
extern "C" {
#include "user_interface.h"
}

String alınan = "";
String mesaj = "";
char harf;
bool alıs = false;
bool gonderildi = true;
int sayac = 32;
long zaman = 0;

Adafruit_SSD1306 display(128, 64, &Wire, -1);


void setup() {

  pinMode(D9, INPUT_PULLUP);  //enc a
  pinMode(D0, INPUT_PULLUP);  //enc b
  pinMode(D10, OUTPUT);       //notification


  Wire.begin(D4, D3);  //oled

  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.setFont(&Orbitron_Bold_14);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  LoRa.setPins(D1, D8, D2);
  while (!LoRa.begin(433.175E6)) {  //frequency (E = 10^n, 10^6 for mhz)
  }
  //LORA CONFIGRATION, OPTIMIZE FOR YOUR USE, BOTH DEVICES MUST HAVE SAME CONFIGRATION, CHECK https://www.semtech.com/design-support/lora-calculator
  LoRa.setSyncWord(0x26);                       // sync code
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);  //module max tx power
  LoRa.setSignalBandwidth(62.5E3);              //small bw = high ranges; frequency shift may cause inconsistent readings at low BW, try to increase. 7.8kHz, 10.4kHz, 15.6kHz, 20.8kHz, 31.25kHz, 41.7kHz, 62.5kHz, 125kHz, 250kHz, 500kHz
  LoRa.setSpreadingFactor(12);                  //fast,poor range 0-12 slow,incresed range
  LoRa.enableCrc();                             // error correction
  LoRa.setCodingRate4(8);                       // error correction 4-8
  LoRa.setPreambleLength(32);                   //proportional to range,increases airtime
  LoRa.enableLowDataRateOptimize();
  LoRa.onReceive(goster);  // funciton that works when reciving data
  LoRa.setGain(6);         //0 auto, 1-6 gain. noise increases proportionaly
  LoRa.onTxDone(tx_done);
  LoRa.receive();
}



void loop() {
  encoder();  //handle enc & buttons
  oled();     //display
  WiFi.mode(WIFI_OFF);
}

void tx_done() {
  gonderildi = true;
  delay(100);
  LoRa.receive();  //turning into RX mode after complating transmition
  delay(100);
}

void oled() {
  if (!alıs) {  //is there incoming data?
    display.clearDisplay();
    display.setFont(&Orbitron_Bold_14);
    display.setCursor(110, 10);
    display.println(harf);
    display.setCursor(0, 30);
    display.println(">_" + mesaj);
    display.setCursor(0, 50);
    display.setFont(&Orbitron_Bold_9);
    display.println("A0:" + String(analogRead(A0)) + " enc:" + String(sayac) + " TX:" + String(gonderildi));
    display.display();

  } else {  //display data

    display.clearDisplay();
    display.setCursor(0, 20);
    display.println(alınan);
    display.display();
    for (int i = 0; i < 5; i++) {  //notification
      digitalWrite(D10, HIGH);
      delay(100);
      digitalWrite(D10, LOW);
      delay(200);
    }
    delay(3000);

    alıs = false;
  }
}


void encoder() {

  if (digitalRead(D9) == 1 && digitalRead(D0) == 0) {  //CW
    while (digitalRead(D9) == 1 && digitalRead(D0) == 0) {
    }
    if (digitalRead(D9) == 0 && digitalRead(D0) == 0) {
      sayac++;
    }
  }

  if (digitalRead(D9) == 0 && digitalRead(D0) == 1) {  //CCW
    while (digitalRead(D9) == 0 && digitalRead(D0) == 1) {
    }
    if (digitalRead(D9) == 0 && digitalRead(D0) == 0) {
      sayac--;
    }
  }


  harf = char(constrain(sayac + 33, 33, 126));  // generate character

  if ((900 > analogRead(A0)) && (analogRead(A0) > 650)) {  //delete button
    digitalWrite(D10, HIGH);

    digitalWrite(D10, LOW);
    mesaj.remove(mesaj.length() - 1);
    while ((900 > analogRead(A0)) && (analogRead(A0) > 650)) {
    }
  }

  if (analogRead(A0) > 900) {  //char button
    mesaj = mesaj + harf;
    digitalWrite(D10, HIGH);

    digitalWrite(D10, LOW);
    while (analogRead(A0) > 900) {
      ;
    }
  }

  if ((350 > analogRead(A0)) && (analogRead(A0) > 150)) {  //send button
    digitalWrite(D10, HIGH);

    digitalWrite(D10, LOW);
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println(">" + mesaj + "<");
    display.display();

    if (gonderildi) {  //checking if transmition of last package complated
      LoRa.beginPacket();
      LoRa.print(mesaj);  //sending message
      gonderildi = false;
      LoRa.endPacket(true);

      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("<" + mesaj + ">");
      display.display();
      delay(2000);

      mesaj = "";
    } else {//waiting for ending of previous transmission
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Lutfen Bekleyin");
      display.display();
      delay(2000);
    }

    sayac = 32;  // re-arrange character to 'A'

    while ((350 > analogRead(A0)) && (analogRead(A0) > 150)) {
      ;
    }
  }

  if ((650 > analogRead(A0)) && (analogRead(A0) > 350)) {  //space button
    mesaj = mesaj + " ";
    digitalWrite(D10, HIGH);

    digitalWrite(D10, LOW);
    while ((650 > analogRead(A0)) && (analogRead(A0) > 350)) {
      ;
    }
  }
}

void goster(int packetSize) {  //handle the incoming message packets

  if (packetSize != 0) {
    alınan = LoRa.readString() + "\nrssi:" + LoRa.packetRssi();  // message+rssi(rx power)
  }

  alıs = true;
}
