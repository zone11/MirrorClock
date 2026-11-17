/*******************************************************************************************
 ** Autor: Christian Egger
 ** Original Autor: Alf Müller                                                            **
 ** Datum: Januar 2026                                                                    **
 ** Version: 1.0                                                                          **
 **
 ** New release using PlatformIO instead of Arduino IDE
 *******************************************************************************************
*/

#include <Arduino.h>

/************************( Importieren der genutzten Bibliotheken )************************/

#include <ESP8266WiFi.h>                    //https://github.com/esp8266/Arduino
#include "WiFiManager.h"                    //https://github.com/tzapu/WiFiManager
#include <Timezone.h>                       //https://github.com/JChristensen/Timezone
#include <TimeLib.h>                        //https://github.com/PaulStoffregen/Time
#include <WiFiUdp.h>
#include <NeoPixelBrightnessBus.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Wire.h>
#include <BH1750.h>                         //https://github.com/claws/BH1750



int h, m, luxSensor, taste;                 // h, m = lokale Zeit
long messtimer = 0;
long timerinterval = 1000;
long aktuellertimer = 1000;
long menuinterval = 6000;
long menutimer = 0;
int aktivmenu = 0;

// - BH1750 - //
BH1750 lightMeter;
uint16_t lux;
int hellmodus = 0;                          // 0 = Auto, 1 = Manuell
int luxkorr = 0;
int luxstufe = 2;


// - IRremoteESP8266. - //
//const int RECV_PIN = 14;
const int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;
unsigned long key_value = 0;

// - NeoPixelBus. - //
const uint16_t PixelCount = 114;
const uint8_t PixelPin = 2;                 // wird ignoriert: die DMA-Methode nutzt immer GPIO3 (RX)
NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
int aktivcolor = 1;

RgbColor black(0);
RgbColor white(200, 200, 200);
int regenbogenfarbe= 0;
unsigned long regenbogentimer;
unsigned long regenbogeninterval = 100;

// NTP Serverpool für die Schweiz:
static const char ntpServerName[] = "ch.pool.ntp.org";    //Finde lokale Server unter http://www.pool.ntp.org/zone/ch
const int timeZone = 0;                     // 0 wenn mit <Timezone.h> gearbeitet wird sonst stimmt es nachher nicht
WiFiUDP Udp;
unsigned int localPort = 8888;              // lokaler Port zum Abhören von UDP-Paketen

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

// - Timezone. - //
// Bearbeiten Sie diese Zeilen entsprechend Ihrer Zeitzone und Sommerzeit.
// TimeZone Einstellungen Details https://github.com/JChristensen/Timezone
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Time (Frankfurt, Paris)
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Time (Frankfurt, Paris)
Timezone CE(CEST, CET);

// Forward declarations
void helligkeit();
void translateIR(decode_results *results);
void tastecheck();
void menuein(int menuwert);
void menuanzeigeA();
void menuanzeigeM();
void clearDot();
void pixeltest();
void neo(int led);
void wort();
void sym_showwifi();
void sym_mm();
void sym_aa();
void sym_stufe();
void itis();
void m5();
void m15();
void m10();
void m20();
void before();
void after();
void half();
void min1();
void min2();
void min3();
void min4();
void houres(int htemp);
void timebuilder();
void minutes();
void lokaleZeit();
void heller();
void dunkler();
void setlux();

void setup() {
  Serial.begin(115200);
  strip.Begin();
#ifdef LED_TEST
  pixeltest();                              // nur mit Build-Flag -DLED_TEST
#endif
  sym_showwifi();
  strip.Show();
  WiFiManager wifiManager;
  //wifiManager.resetSettings();            //mit diesem befehl kannst die gespeicherten werte löschen
  wifiManager.autoConnect("MirrorClock");
  Serial.println("verbunden!");
  Udp.begin(localPort);
  Serial.print("lokaler Port: ");
  Serial.println(Udp.localPort());
  Serial.println("Warten auf die Synchronisation");
  setSyncProvider(getNtpTime);               // getNtpTime() setzt das Intervall: 60 s nach Fehler, 86400 s (1 Tag) nach Erfolg

  irrecv.enableIRIn();  // Start the receiver
  Wire.begin(D4, D3);                       // (D3,SCL) und (D4,SDA) pins
  lightMeter.begin();

}

time_t prevDisplay = 0; // wenn die Digitaluhr angezeigt wurde

void loop()
{
  aktuellertimer = millis();
  if (timeStatus() != timeNotSet) {             // wenn die Zeit über NTP gesetzt wurde
    lokaleZeit();
    wort();
  }

  helligkeit();
  if (aktuellertimer - menutimer > menuinterval) {
    aktivmenu = 0;
  }

  if (irrecv.decode(&results)) {
    if (results.value == 0XFFFFFFFF) {
      results.value = key_value;
    }
    translateIR(&results);
    key_value = results.value;
    irrecv.resume();
  }
  strip.Show();                             // einziges Show() pro Durchlauf
  delay(20);
}


/************************( helligkeit.ino )************************/

void helligkeit() {
  if (aktuellertimer - messtimer > timerinterval) {
    messtimer = millis();
    if (hellmodus == 0) {                    // keine messung auf manuellem modus (1)
      float messung = lightMeter.readLightLevel();
      if (messung >= 0) {                    // -1/-2 = Sensorfehler, letzten Wert behalten
        luxSensor = messung;
      }
    }
  }
  lux = map(constrain(luxSensor, 0, 10), 0, 10, 40, 255);  // vor map begrenzen, sonst Überlauf in uint16_t

  if (hellmodus == 0) {
    strip.SetBrightness(lux);
  }
  if (hellmodus == 1) {
    if (  aktivmenu == 1) {
    setlux();
    menuanzeigeM();
    }
    strip.SetBrightness(luxkorr);
  }
}

void heller() {
  menutimer = millis();
  clearDot();
  if (luxstufe <= 4) {
    luxstufe = luxstufe + 1;
  }
  menuanzeigeM();
  setlux();
}

void dunkler() {
  menutimer = millis();
  clearDot();
  if (luxstufe >= 2) {
    luxstufe = luxstufe - 1;
  }
  setlux();
  menuanzeigeM();
}


void setlux() { //manuell
  switch (luxstufe) {
    case 1 :
      luxkorr = 30;
      break;
    case 2 :
      luxkorr = 60;
      break;
    case 3 :
      luxkorr = 100;
      break;
    case 4 :
      luxkorr = 150;
      break;
    case 5 :
      luxkorr = 255;
      break;
    default:
      luxkorr = 30;
      break;
  }
}


/************************( ir.ino )************************/

void translateIR(decode_results *results)
{
  switch (results->value) {
    case 0xFD00FF:            // 1
      taste = 1;
      break;
    case 0xFFA25D:
      taste = 1;
      break;
    case 0xFD807F:            // 2
      taste = 2;
      break;
    case 0xFF629D:
      taste = 2;
      break;
    case 0xFD40BF:            // 3
      taste = 3;
      break;
    case 0xFFE21D:
      taste = 3;
      break;
    case 0xFD20DF:            // 4
      taste = 4;
      break;
    case 0xFF22DD:
      taste = 4;
      break;
    case 0xFDA05F:            // 5
      taste = 5;
      break;
    case 0xFF02FD:
      taste = 5;
      break;
    case 0xFD609F:            // 6
      taste = 6;
      break;
    case 0xFFC23D:
      taste = 6;
      break;
    case 0xFD10EF:            // 7
      taste = 7;
      break;
    case 0xFFE01F:
      taste = 7;
      break;
    case 0xFD906F:            // 8
      taste = 8;
      break;
    case 0xFFA857:
      taste = 8;
      break;
    case 0xFD50AF:            // 9
      taste = 9;
      break;
    case 0xFF906F:
      taste = 9;
      break;
    case 0xFDB04F:            // 0
      taste = 0;
      break;
    case 0xFF9867:
      taste = 0;
      break;
    case 0xFD708F:            // * = 11
      taste = 11;
      break;
    case 0xFF6897:
      taste = 11;
      break;
    case 0xFFB04F:            // # = 12
      taste = 12;
      break;
    case 0x2C739B:
      taste = 12;
      break;
    case 0xFD8877:            // hoch 13
      taste = 13;
      break;
    case 0xFF18E7:
      taste = 13;
      break;
    case 0xFD28D7:            // links = 14
      taste = 14;
      break;
    case 0xFF10EF:
      taste = 14;
      break;
    case 0xFDA857:            // OK = 15
      taste = 15;
      break;
    case 0xFF38C7:
      taste = 15;
      break;
    case 0xFD6897:            // rechts = 16
      taste = 16;
      break;
    case 0xFF5AA5:
      taste = 16;
      break;
    case 0xFD9867:            // runter 17
      taste = 17;
      break;
    case 0xFF4AB5:
      taste = 17;
      break;
  }
  tastecheck();
}

void tastecheck() {
  switch (taste) {
    case 1:               // Farbe 1
      aktivcolor = 1;
      break;
    case 2:               // Farbe 2
      aktivcolor = 2;
      break;
    case 3:               // Farbe 3
      aktivcolor = 3;
      break;
    case 4:               // Farbe 4
      aktivcolor = 4;
      break;
    case 5:               // Farbe 5
      aktivcolor = 5;
      break;
    case 6:               // Farbe 6
      aktivcolor = 6;
      break;
    case 7:               // Farbe 7
      aktivcolor = 7;
      break;
    case 8:               // Farbe 8
      aktivcolor = 8;
      break;
    case 9:               // Farbe 9
      aktivcolor = 9;
      break;
    case 0:               // Farbe 0
      aktivcolor = 0;
      break;
    case 11:               // Farbe RGB alle
      aktivcolor = 88;
      break;
    case 12:               // Farbe Regenbogenfarben
      aktivcolor = 77;
      break;
    case 13:               // Farbe Regenbogenfarben
      if (aktivmenu == 1) {
        heller();
      }
      break;
    case 14:               // Farbe Regenbogenfarben
      menuein(1);
      break;
    case 15:               // OK
      if (aktivcolor == 88) { //wenn rgb läuft
        aktivcolor = 99;
      }
      break;
    case 16:               // Farbe Regenbogenfarben
      menuein(2);
      break;
    case 17:               // Farbe Regenbogenfarben
      if (aktivmenu == 1) {
        dunkler();
      }
      break;
  }
  taste = 99;
}


/************************( menu.ino )************************/

void menuein(int menuwert) {
  aktivmenu = 1;
  switch (menuwert) {
    case 1:            // Menu Helligkeit
      menutimer = millis(); // menuanzeige für 8 sekunden weiterführen wenn was gedrückt wurde
      menuanzeigeA();
      break;
    case 2:            // Menu Helligkeit
      menutimer = millis(); // menuanzeige für 8 sekunden weiterführen wenn was gedrückt wurde
      menuanzeigeM();
      break;
  }
}

void menuanzeigeA() { // automatische helligkeit
  clearDot();
  sym_aa();
  hellmodus = 0;
  delay(5);
  strip.Show();
  delay(10);
}

void menuanzeigeM() { // Manueller helligkeit
  clearDot();
  hellmodus = 1;
  sym_mm();
  sym_stufe();
  delay(5);
  strip.Show();
  delay(10);
}


/************************( pixel.ino )************************/

void clearDot() {
  strip.ClearTo(black);
}

void pixeltest() {
  for (int i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, HslColor((0) / 360.0f, 1.0f, 0.25f));
    strip.Show();
    delay(50); // Delay for a period of time (in milliseconds).
    clearDot();
  }
  for (int i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, HslColor((120) / 360.0f, 1.0f, 0.25f));
    strip.Show();
    delay(50); // Delay for a period of time (in milliseconds).
    clearDot();
  }
  for (int i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, HslColor((240) / 360.0f, 1.0f, 0.25f));
    strip.Show();
    delay(50); // Delay for a period of time (in milliseconds).
    clearDot();
  }
}

// farbwechsel
void neo(int led) {
  switch (aktivcolor) {
    case 1: //up
      strip.SetPixelColor(led, HslColor((150) / 360.0f, 1.0f, 0.25f));
      break;
    case 2: //up
      strip.SetPixelColor(led, HslColor((130) / 360.0f, 1.0f, 0.25f));
      break;
    case 3: //up
      strip.SetPixelColor(led, HslColor((30) / 360.0f, 1.0f, 0.25f));
      break;
    case 4: //up
      strip.SetPixelColor(led, HslColor((0) / 360.0f, 1.0f, 0.25f));
      break;
    case 5: //up
      strip.SetPixelColor(led, white);
      break;
    case 6: //up
      strip.SetPixelColor(led, HslColor((100) / 360.0f, 1.0f, 0.25f));
      break;
    case 7: //up
      strip.SetPixelColor(led, HslColor((190) / 360.0f, 1.0f, 0.25f));
      break;
    case 8: //up
      strip.SetPixelColor(led, HslColor((300) / 360.0f, 1.0f, 0.25f));
      break;
    case 9: //up
      strip.SetPixelColor(led, HslColor((50) / 360.0f, 1.0f, 0.25f));
      break;
    case 0: //up
      strip.SetPixelColor(led, HslColor((327) / 360.0f, 1.0f, 0.25f));
      break;
    case 77: //up
      // 0-360 sind farben - position pixel
      if (millis() - regenbogentimer  > regenbogeninterval) {
        regenbogentimer = millis();
        regenbogenfarbe = (regenbogenfarbe + 1);
        if (regenbogenfarbe == 360) {
          regenbogenfarbe = 0;
        }
      }
      strip.SetPixelColor(led, HslColor((regenbogenfarbe - led) / 360.0f, 1.0f, 0.25f));
      break;
    case 88: //up
      // 0-360 sind farben - position pixel
      if (millis() - regenbogentimer  > regenbogeninterval) {
        regenbogentimer = millis();
        regenbogenfarbe = (regenbogenfarbe + 1);
        if (regenbogenfarbe == 360) {
          regenbogenfarbe = 0;
        }
      }
      strip.SetPixelColor(led, HslColor((regenbogenfarbe) / 360.0f, 1.0f, 0.25f));
      break;
    case 99: //RGB Farbe einfrieren
      strip.SetPixelColor(led, HslColor((regenbogenfarbe) / 360.0f, 1.0f, 0.25f));
      break;
  }
}


void wort() {
  if (aktivmenu == 0) {
    timebuilder();
  }
}


/************************( symbole.ino )************************/

void sym_showwifi() {
  neo(44);
  neo(45);
  neo(53);
  neo(54);
}

void sym_mm() { //  male ein M für Manuell oben links
  neo(0);
  neo(1);
  neo(2);
  neo(3);
  neo(4);
  neo(18);
  neo(22);
  neo(38);
  neo(40);
  neo(41);
  neo(42);
  neo(43);
  neo(44);
}
void sym_aa() { //  male ein A für Auto oben links

  neo(1);
  neo(2);
  neo(3);
  neo(4);
  neo(17);
  neo(19);
  neo(20);
  neo(22);
  neo(37);
  neo(39);
  neo(41);
  neo(42);
  neo(43);
  neo(44);
}
void sym_stufe() {
  switch (luxstufe) {
    case 1 :            // 1
      neo(69);
      break;
    case 2 :            // 2
      neo(69);
      neo(70);
      neo(71);
      break;
    case 3 :            // 3
      neo(69);
      neo(70);
      neo(71);
      neo(87);
      neo(88);
      neo(89);
      break;
    case 4 : // 4
      neo(69);
      neo(70);
      neo(71);
      neo(87);
      neo(88);
      neo(89);
      neo(90);
      neo(91);
      neo(92);
      neo(93);
      break;
    case 5 :            // 5
      neo(69);
      neo(70);
      neo(71);
      neo(87);
      neo(88);
      neo(89);
      neo(90);
      neo(91);
      neo(92);
      neo(93);
      neo(105);
      neo(109);
      neo(108);
      neo(107);
      neo(106);
      break;
  }
}


/************************( woerter.ino )************************/

void itis() { //
  neo(0);
  neo(19);
  neo(39);
  neo(40);
  neo(59);
  neo(60);
}
void m5() { //
  neo(80);
  neo(99);
  neo(100);
}
void m15() { //
  neo(1);
  neo(18);
  neo(21);
  neo(38);
  neo(41);
  neo(58);
}
void m10() { //
  neo(81);
  neo(98);
  neo(101);
}
void m20() { //
  neo(2);
  neo(17);
  neo(22);
  neo(37);
  neo(42);
  neo(57);
}
void before() { //
  neo(82);
  neo(97);
  neo(102);
}
void after() { //
  neo(3);
  neo(16);
}
void half() { //
  neo(36);
  neo(43);
  neo(56);
  neo(63);
  neo(76);
}
void min1() { //
  neo(112);
}
void min2() { //
  neo(112);
  neo(111);
}
void min3() { //
  neo(110);
  neo(111);
  neo(112);
}
void min4() { //
  neo(110);
  neo(111);
  neo(112);
  neo(113);
}
void houres(int htemp) {
  if (h >= 12) {
    htemp = (h - 12);
  }
  if (m > 24){
    htemp = htemp+1;
  }

  switch (htemp) {
    case 1:
      neo(4);
      neo(15);
      neo(24);
      break;
    case 2:
      neo(35);
      neo(44);
      neo(55);
      neo(64);
      break;
    case 3:
      neo(84);
      neo(95);
      neo(104);
      break;
    case 4:
      neo(5);
      neo(14);
      neo(25);
      neo(34);
      neo(45);
      break;
    case 5:
      neo(54);
      neo(65);
      neo(74);
      neo(85);
      break;
    case 6:
      neo(6);
      neo(13);
      neo(26);
      neo(33);
      neo(46);
      neo(53);
      break;
    case 7:
      neo(66);
      neo(73);
      neo(86);
      neo(93);
      neo(106);
      break;
    case 8:
      neo(7);
      neo(12);
      neo(27);
      neo(32);
      neo(47);
      break;
    case 9:
      neo(52);
      neo(67);
      neo(72);
      neo(87);
      break;
    case 10:
      neo(8);
      neo(11);
      neo(28);
      neo(31);
      break;
    case 11:
      neo(71);
      neo(88);
      neo(91);
      neo(108);
      break;
    case 12:
      neo(9);
      neo(10);
      neo(29);
      neo(30);
      neo(49);
      neo(50);
      break;
      case 0:
      neo(9);
      neo(10);
      neo(29);
      neo(30);
      neo(49);
      neo(50);
      break;
  }
}


/************************( zeit.ino )************************/

void timebuilder() {
  clearDot();
  itis(); // ES ISCH
  minutes();
}

void minutes() {
  switch (m) {
    case 0:
      houres(h);
      break;
    case 1:
      houres(h);
      min1();
      break;
    case 2:
      houres(h);
      min2();
      break;
    case 3:
      houres(h);
      min3();
      break;
    case 4:
      houres(h);
      min4();
      break;
    case 5:
      houres(h);
      m5();
      after();
      break;
    case 6:
      houres(h);
      m5();
      after();
      min1();
      break;
    case 7:
      houres(h);
      m5();
      after();
      min2();
      break;
    case 8:
      houres(h);
      m5();
      after();
      min3();
      break;
    case 9:
      houres(h);
      m5();
      after();
      min4();
      break;
    case 10:
      houres(h);
      m10();
      after();
      break;
    case 11:
      houres(h);
      m10();
      after();
      min1();
      break;
    case 12:
      houres(h);
      m10();
      after();
      min2();
      break;
    case 13:
      houres(h);
      m10();
      after();
      min3();
      break;
    case 14:
      houres(h);
      m10();
      after();
      min4();
      break;
    case 15:
      houres(h);
      m15();
      after();
      break;
    case 16:
      houres(h);
      m15();
      after();
      min1();
      break;
    case 17:
      houres(h);
      m15();
      after();
      min2();
      break;
    case 18:
      houres(h);
      m15();
      after();
      min3();
      break;
    case 19:
      houres(h);
      m15();
      after();
      min4();
      break;
    case 20:
      houres(h);
      m20();
      after();
      break;
    case 21:
      houres(h);
      m20();
      after();
      min1();
      break;
    case 22:
      houres(h);
      m20();
      after();
      min2();
      break;
    case 23:
      houres(h);
      m20();
      after();
      min3();
      break;
    case 24:
      houres(h);
      m20();
      after();
      min4();
      break;
    case 25:
     houres(h);
      m5();
      before();
      half();
      break;
    case 26:
     houres(h);
      m5();
      before();
      half();
      min1();
      break;
    case 27:
     houres(h);
      m5();
      before();
      half();
      min2();
      break;
    case 28:
     houres(h);
      m5();
      before();
      half();
      min3();
      break;
    case 29:
     houres(h);
      m5();
      before();
      half();
      min4();
      break;
    case 30:
     houres(h);
      half();
      break;
    case 31:
     houres(h);
      half();
      min1();
      break;
    case 32:
     houres(h);
      half();
      min2();
      break;
    case 33:
     houres(h);
      half();
      min3();
      break;
    case 34:
     houres(h);
      half();
      min4();
      break;
    case 35:
     houres(h);
      half();
      m5();
      after();
      break;
    case 36:
     houres(h);
      half();
      m5();
      after();
      min1();
      break;
    case 37:
     houres(h);
      half();
      m5();
      after();
      min2();
      break;
    case 38:
     houres(h);
      half();
      m5();
      after();
      min3();
      break;
    case 39:
     houres(h);
      half();
      m5();
      after();
      min4();
      break;
    case 40:
     houres(h);
      m20();
      before();
      break;
    case 41:
     houres(h);
      m20();
      before();
      min1();
      break;
    case 42:
     houres(h);
      m20();
      before();
      min2();
      break;
    case 43:
     houres(h);
      m20();
      before();
      min3();
      break;
    case 44:
     houres(h);
      m20();
      before();
      min4();
      break;
    case 45:
     houres(h);
      m15();
      before();
      break;
    case 46:
     houres(h);
      m15();
      before();
      min1();
      break;
    case 47:
     houres(h);
      m15();
      before();
      min2();
      break;
    case 48:
     houres(h);
      m15();
      before();
      min3();
      break;
    case 49:
     houres(h);
      m15();
      before();
      min4();
      break;
    case 50:
     houres(h);
      m10();
      before();
      break;
    case 51:
     houres(h);
      m10();
      before();
      min1();
      break;
    case 52:
     houres(h);
      m10();
      before();
      min2();
      break;
    case 53:
     houres(h);
      m10();
      before();
      min3();
      break;
    case 54:
     houres(h);
      m10();
      before();
      min4();
      break;
    case 55:
     houres(h);
      m5();
      before();
      break;
    case 56:
     houres(h);
      m5();
      before();
      min1();
      break;
    case 57:
     houres(h);
      m5();
      before();
      min2();
      break;
    case 58:
     houres(h);
      m5();
      before();
      min3();
      break;
    case 59:
     houres(h);
      m5();
      before();
      min4();
      break;

  }
}


void lokaleZeit()
{
  time_t tT = now();
  time_t tTlocal = CE.toLocal(tT);
  h = hour(tTlocal);
  m = minute(tTlocal);
}



/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP-Zeit in den ersten 48 Bytes der Nachricht
byte packetBuffer[NTP_PACKET_SIZE]; //Puffer für eingehende und ausgehende Pakete

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip Adresse

  while (Udp.parsePacket() > 0) ; // alle zuvor empfangenen Pakete verwerfen
  Serial.println("Transmit NTP Request");
  // einen zufälligen Server aus dem Pool holen
  if (!WiFi.hostByName(ntpServerName, ntpServerIP)) {
    Serial.println("NTP Server nicht gefunden");
    setSyncInterval(60);                    // bald erneut versuchen statt erst in 1 Tag
    return 0;
  }
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // Paket in den Puffer einlesen
      unsigned long secsSince1900;
      // vier Bytes ab Position 40 in eine lange Ganzzahl umwandeln
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      setSyncInterval(86400);               // 1 Tag bis zum nächsten Abgleich
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("keine NTP Antwort");
  setSyncInterval(60);                      // bald erneut versuchen statt erst in 1 Tag
  return 0; // gibt 0 zurück, wenn die Zeit nicht ermittelt werden kann.
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // alle Bytes im Puffer auf 0 setzen
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialisieren von Werten, die für die Bildung von NTP-Requests benötigt werden.
  // (siehe URL oben für Details zu den Paketen)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // alle NTP-Felder wurden jetzt mit Werten versehen
  // Sie können ein Paket senden, das einen Zeitstempel anfordert.:
  Udp.beginPacket(address, 123); //NTP-Requests sollen auf Port 123 erfolgen
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
