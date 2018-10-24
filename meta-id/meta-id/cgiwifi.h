#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"

enum { wifiIsDisconnected, wifiIsConnected, wifiGotIP };
typedef void(*WifiStateChangeCb)(uint8_t wifiStatus);

int cgiWiFiScan(HttpdConnData *connData);
int cgiWifiInfo(HttpdConnData *connData);
int cgiWiFi(HttpdConnData *connData);
int cgiWiFiConnect(HttpdConnData *connData);
int cgiWiFiSetMode(HttpdConnData *connData);
int cgiWiFiConnStatus(HttpdConnData *connData);
int cgiWiFiSpecial(HttpdConnData *connData);
int cgiApSettingsChange(HttpdConnData *connData);
int cgiApSettingsInfo(HttpdConnData *connData);
void configWifiIP();
void wifiInit(void);
void wifiAddStateChangeCb(WifiStateChangeCb cb);
int checkString(char *str);
void ICACHE_FLASH_ATTR wifiConfigWipe();

extern uint8_t wifiState;

int wifiGetApCount();
void wifiGetApName(int, char *);
int wifiSignalStrength(int);
void connectToNetwork(char *, char *);
void wifiStartScan();
void statusWifiUpdate(uint8_t state);

#endif
