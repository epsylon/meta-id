#ifndef CGIMETA_H
#define CGIMETA_H

#include "httpd.h"

int cgiMetaUserPass(HttpdConnData *connData);
int cgiMetaAuth(HttpdConnData *connData);
int cgiMetaCheckAuth(HttpdConnData *connData);
int cgiMetaCheckAuthCgi(HttpdConnData *connData);
int cgiMetaHome(HttpdConnData *connData);
int cgiMetaDump(HttpdConnData *connData);
int cgiMetaWav(HttpdConnData *connData);
int cgiMetaGpio(HttpdConnData *connData);
int cgiMetaGetSignal(HttpdConnData *connData);
//void cgiMetaInit();
int metaCheckHash (int32 hash);
int cgiMetaLogout(HttpdConnData *connData);
int cgiMetaGetSSID(HttpdConnData *connData);
int ICACHE_FLASH_ATTR meta_init_gpio();
int ICACHE_FLASH_ATTR cgiMetaSend(HttpdConnData *connData) ;
int ICACHE_FLASH_ATTR cgiMetaFetch(HttpdConnData *connData) ;
int ICACHE_FLASH_ATTR cgiMetaGetTimers(HttpdConnData *connData) ;
int ICACHE_FLASH_ATTR cgiMetaState(HttpdConnData *connData) ;
extern char* rst_codes[7];
extern char* flash_maps[7];

#endif // CGIMETA_H
