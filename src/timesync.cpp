#include "timesync.h"

#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFiUdp.h>

// Replaced by the core's configTime() in a later phase.

namespace {

const char NTP_SERVER[] = "ch.pool.ntp.org";
const unsigned int LOCAL_PORT = 8888;
const int NTP_PACKET_SIZE = 48;
const unsigned long NTP_TIMEOUT_MS = 1500;
const time_t RETRY_INTERVAL_S = 60;
const time_t SYNC_INTERVAL_S = 86400;

// Central European Time with daylight saving
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
Timezone zone(CEST, CET);

WiFiUDP udp;
uint8_t packet[NTP_PACKET_SIZE];

void sendRequest(IPAddress &address) {
  memset(packet, 0, NTP_PACKET_SIZE);
  packet[0] = 0b11100011;  // LI, version, mode
  packet[1] = 0;           // stratum
  packet[2] = 6;           // polling interval
  packet[3] = 0xEC;        // peer clock precision
  packet[12] = 49;         // reference ID
  packet[13] = 0x4E;
  packet[14] = 49;
  packet[15] = 52;
  udp.beginPacket(address, 123);
  udp.write(packet, NTP_PACKET_SIZE);
  udp.endPacket();
}

// Sync provider for TimeLib. Returns UTC, or 0 on failure.
// Sets the next sync: 60 s after a failure, one day after success.
time_t getNtpTime() {
  while (udp.parsePacket() > 0) {}  // drop old packets

  IPAddress server;
  if (!WiFi.hostByName(NTP_SERVER, server)) {
    Serial.println("NTP server not found");
    setSyncInterval(RETRY_INTERVAL_S);
    return 0;
  }
  Serial.print("NTP request to ");
  Serial.print(NTP_SERVER);
  Serial.print(" (");
  Serial.print(server);
  Serial.println(")");
  sendRequest(server);

  unsigned long start = millis();
  while (millis() - start < NTP_TIMEOUT_MS) {
    if (udp.parsePacket() >= NTP_PACKET_SIZE) {
      udp.read(packet, NTP_PACKET_SIZE);
      unsigned long secsSince1900 = (unsigned long)packet[40] << 24 | (unsigned long)packet[41] << 16 |
                                    (unsigned long)packet[42] << 8 | (unsigned long)packet[43];
      Serial.println("NTP response received");
      setSyncInterval(SYNC_INTERVAL_S);
      return secsSince1900 - 2208988800UL;
    }
  }
  Serial.println("No NTP response");
  setSyncInterval(RETRY_INTERVAL_S);
  return 0;
}

}  // namespace

void timesyncBegin() {
  udp.begin(LOCAL_PORT);
  setSyncProvider(getNtpTime);  // syncs right away
}

bool timesyncLocalTime(int &hour, int &minute) {
  if (timeStatus() == timeNotSet) {
    return false;
  }
  time_t local = zone.toLocal(now());
  hour = ::hour(local);
  minute = ::minute(local);
  return true;
}
