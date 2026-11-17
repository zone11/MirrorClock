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
#include <initializer_list>

/************************( Importieren der genutzten Bibliotheken )************************/

#include <ESP8266WiFi.h>                    //https://github.com/esp8266/Arduino
#include "WiFiManager.h"                    //https://github.com/tzapu/WiFiManager
#include <Timezone.h>                       //https://github.com/JChristensen/Timezone
#include <TimeLib.h>                        //https://github.com/PaulStoffregen/Time
#include <WiFiUdp.h>
#include <NeoPixelBusLg.h>
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Wire.h>
#include <BH1750.h>                         //https://github.com/claws/BH1750
#ifdef WEB_TASTE
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);                // Test ohne Fernbedienung, nur mit Build-Flag -DWEB_TASTE
#endif



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
NeoPixelBusLg<NeoGrbFeature, Neo800KbpsMethod, NeoGammaNullMethod> strip(PixelCount, PixelPin);  // ohne Gamma = gleiche Farben wie bisher
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
void leds(std::initializer_list<uint8_t> liste);
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
void houres();
void timebuilder();
void minutes();
void lokaleZeit();
void heller();
void dunkler();
void setlux();
void speichern();
void laden();

void setup() {
  Serial.begin(115200);
  EEPROM.begin(16);
  laden();                                  // Farbe und Helligkeit vom letzten Mal
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
#ifdef WEB_TASTE
  // /taste?nr=5 drückt Taste 5 wie die Fernbedienung (0-9, 11-17); ohne nr nur Status
  server.on("/taste", []() {
    if (server.hasArg("nr")) {
      String nr = server.arg("nr");
      int wert = nr.toInt();
      if (nr != String(wert) || wert < 0 || wert > 17 || wert == 10) {
        server.send(400, "text/plain", "nr muss 0-9 oder 11-17 sein\n");
        return;
      }
      taste = wert;
      tastecheck();
    }
    server.send(200, "text/plain", "aktivcolor=" + String(aktivcolor) + " regenbogenfarbe=" + String(regenbogenfarbe)
                + " hellmodus=" + String(hellmodus) + " luxstufe=" + String(luxstufe) + " aktivmenu=" + String(aktivmenu) + "\n");
  });
  server.begin();
#endif

}

time_t prevDisplay = 0; // wenn die Digitaluhr angezeigt wurde

void loop()
{
  aktuellertimer = millis();
#ifdef WEB_TASTE
  server.handleClient();
#endif
  helligkeit();                                 // vor dem Zeichnen: SetLuminance wirkt nur auf neu gesetzte Pixel
  if (timeStatus() != timeNotSet) {             // wenn die Zeit über NTP gesetzt wurde
    lokaleZeit();
    wort();
  }

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
    strip.SetLuminance(lux);
  }
  if (hellmodus == 1) {
    if (  aktivmenu == 1) {
    setlux();
    menuanzeigeM();
    }
    strip.SetLuminance(luxkorr);
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
  const int stufen[] = {30, 60, 100, 150, 255};   // Stufe 1-5
  luxkorr = stufen[constrain(luxstufe, 1, 5) - 1];
}


/************************( ir.ino )************************/

void translateIR(decode_results *results)
{
  taste = 99;               // unbekannter Code = keine Aktion
  switch (results->value) { // je zwei Codes: Fernbedienung 0xFD.... und 0xFF....
    case 0xFD00FF: case 0xFFA25D: taste = 1;  break;
    case 0xFD807F: case 0xFF629D: taste = 2;  break;
    case 0xFD40BF: case 0xFFE21D: taste = 3;  break;
    case 0xFD20DF: case 0xFF22DD: taste = 4;  break;
    case 0xFDA05F: case 0xFF02FD: taste = 5;  break;
    case 0xFD609F: case 0xFFC23D: taste = 6;  break;
    case 0xFD10EF: case 0xFFE01F: taste = 7;  break;
    case 0xFD906F: case 0xFFA857: taste = 8;  break;
    case 0xFD50AF: case 0xFF906F: taste = 9;  break;
    case 0xFDB04F: case 0xFF9867: taste = 0;  break;
    case 0xFD708F: case 0xFF6897: taste = 11; break;  // *
    case 0xFFB04F: case 0x2C739B: taste = 12; break;  // #
    case 0xFD8877: case 0xFF18E7: taste = 13; break;  // hoch
    case 0xFD28D7: case 0xFF10EF: taste = 14; break;  // links
    case 0xFDA857: case 0xFF38C7: taste = 15; break;  // OK
    case 0xFD6897: case 0xFF5AA5: taste = 16; break;  // rechts
    case 0xFD9867: case 0xFF4AB5: taste = 17; break;  // runter
  }
  tastecheck();
}

void tastecheck() {
  if (taste <= 9) {         // Farbe 0-9
    aktivcolor = taste;
  }
  switch (taste) {
    case 11:                // Farbe RGB alle
      aktivcolor = 88;
      break;
    case 12:                // Regenbogenfarben
      aktivcolor = 77;
      break;
    case 13:                // hoch: heller
      if (aktivmenu == 1) {
        heller();
      }
      break;
    case 14:                // links: Menu Auto-Helligkeit
      menuein(1);
      break;
    case 15:                // OK: RGB-Farbe einfrieren
      if (aktivcolor == 88) {
        aktivcolor = 99;
      }
      break;
    case 16:                // rechts: Menu manuelle Helligkeit
      menuein(2);
      break;
    case 17:                // runter: dunkler
      if (aktivmenu == 1) {
        dunkler();
      }
      break;
  }
  if (taste != 99) {
    speichern();
  }
}


/************************( einstellungen )************************/

// Farbe und Helligkeit im Flash (EEPROM-Emulation) über einen Neustart behalten
void speichern() {
  EEPROM.put(0, aktivcolor);
  EEPROM.put(4, regenbogenfarbe);           // nötig für die eingefrorene Farbe (99)
  EEPROM.put(8, hellmodus);
  EEPROM.put(12, luxstufe);
  EEPROM.commit();                          // schreibt nur, wenn sich ein Wert geändert hat
}

void laden() {
  int farbe, regenbogen, modus, stufe;
  EEPROM.get(0, farbe);
  EEPROM.get(4, regenbogen);
  EEPROM.get(8, modus);
  EEPROM.get(12, stufe);
  // leeres oder fremdes Flash: ungültige Werte ignorieren, Standardwerte bleiben
  if ((farbe >= 0 && farbe <= 9) || farbe == 77 || farbe == 88 || farbe == 99) {
    aktivcolor = farbe;
  }
  if (regenbogen >= 0 && regenbogen < 360) {
    regenbogenfarbe = regenbogen;
  }
  if (modus == 0 || modus == 1) {
    hellmodus = modus;
  }
  if (stufe >= 1 && stufe <= 5) {
    luxstufe = stufe;
  }
  setlux();                                 // luxkorr für manuellen Modus
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
  for (int farbe : {0, 120, 240}) {         // rot, grün, blau
    for (int i = 0; i < PixelCount; i++) {
      strip.SetPixelColor(i, HslColor(farbe / 360.0f, 1.0f, 0.25f));
      strip.Show();
      delay(50);
      clearDot();
    }
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


// alle LEDs der Liste in der aktiven Farbe setzen
void leds(std::initializer_list<uint8_t> liste) {
  for (uint8_t led : liste) {
    neo(led);
  }
}


/************************( symbole.ino )************************/

void sym_showwifi() {
  leds({44, 45, 53, 54});
}

void sym_mm() { //  male ein M für Manuell oben links
  leds({0, 1, 2, 3, 4, 18, 22, 38, 40, 41, 42, 43, 44});
}

void sym_aa() { //  male ein A für Auto oben links
  leds({1, 2, 3, 4, 17, 19, 20, 22, 37, 39, 41, 42, 43, 44});
}

void sym_stufe() { // Balken für Helligkeitsstufe 1-5, jede Stufe ergänzt die tieferen
  switch (luxstufe) {
    case 5: leds({105, 106, 107, 108, 109}); // fallthrough
    case 4: leds({90, 91, 92, 93});          // fallthrough
    case 3: leds({87, 88, 89});              // fallthrough
    case 2: leds({70, 71});                  // fallthrough
    case 1: leds({69});
  }
}


/************************( woerter.ino )************************/

void itis()   { leds({0, 19, 39, 40, 59, 60}); }      // ES ISCH
void m5()     { leds({80, 99, 100}); }                // FÜF
void m10()    { leds({81, 98, 101}); }                // ZÄH
void m15()    { leds({1, 18, 21, 38, 41, 58}); }      // VIERTU
void m20()    { leds({2, 17, 22, 37, 42, 57}); }      // ZWÄNZG
void before() { leds({82, 97, 102}); }                // VOR
void after()  { leds({3, 16}); }                      // AB
void half()   { leds({36, 43, 56, 63, 76}); }         // HALBI
void min1()   { leds({112}); }                        // Minutenpunkte
void min2()   { leds({111, 112}); }
void min3()   { leds({110, 111, 112}); }
void min4()   { leds({110, 111, 112, 113}); }

void houres() {
  int htemp = h % 12;
  if (m > 24) {             // ab "füf vor halbi" gilt die nächste Stunde
    htemp = htemp + 1;
  }

  switch (htemp) {
    case 1:  leds({4, 15, 24}); break;
    case 2:  leds({35, 44, 55, 64}); break;
    case 3:  leds({84, 95, 104}); break;
    case 4:  leds({5, 14, 25, 34, 45}); break;
    case 5:  leds({54, 65, 74, 85}); break;
    case 6:  leds({6, 13, 26, 33, 46, 53}); break;
    case 7:  leds({66, 73, 86, 93, 106}); break;
    case 8:  leds({7, 12, 27, 32, 47}); break;
    case 9:  leds({52, 67, 72, 87}); break;
    case 10: leds({8, 11, 28, 31}); break;
    case 11: leds({71, 88, 91, 108}); break;
    case 0:
    case 12: leds({9, 10, 29, 30, 49, 50}); break;
  }
}


/************************( zeit.ino )************************/

void timebuilder() {
  clearDot();
  itis(); // ES ISCH
  minutes();
}

void minutes() {
  houres();
  switch (m / 5) {          // Wörter in 5-Minuten-Schritten
    case 1:  m5();   after();  break;
    case 2:  m10();  after();  break;
    case 3:  m15();  after();  break;
    case 4:  m20();  after();  break;
    case 5:  m5();   before(); half(); break;
    case 6:  half(); break;
    case 7:  half(); m5();     after(); break;
    case 8:  m20();  before(); break;
    case 9:  m15();  before(); break;
    case 10: m10();  before(); break;
    case 11: m5();   before(); break;
  }
  switch (m % 5) {          // Minutenpunkte
    case 1: min1(); break;
    case 2: min2(); break;
    case 3: min3(); break;
    case 4: min4(); break;
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
