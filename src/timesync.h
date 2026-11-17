#pragma once

void timesyncBegin();                             // call once WiFi is connected
bool timesyncLocalTime(int &hour, int &minute);   // false until the first successful NTP sync
