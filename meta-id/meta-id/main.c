/*
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
* this notice you can do whatever you want with this stuff. If we meet some day,
* and you think this stuff is worth it, you can buy me a beer in return.
* ----------------------------------------------------------------------------
* Heavily modified and enhanced by Thorsten von Eicken in 2015
* ----------------------------------------------------------------------------
* adapted to meta-id project  by ikujam@ikujam.org (2018)
*
*/

#include <esp8266.h>
#include "httpd.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
//#include "cgipins.h"
//#include "cgitcp.h"
#include "cgimqtt.h"
#include "cgiflash.h"
#include "cgioptiboot.h"
#include "cgimega.h"
#include "cgimeta.h"
#ifdef WEBSERVER
#include "cgiwebserversetup.h"
#endif
#include "auth.h"
#include "espfs.h"
#include "uart.h"
#include "serbridge.h"
//#include "status.h"
#include "serled.h"
#include "console.h"
#include "config.h"
#include "log.h"
#include "gpio.h"
#include "cgiservices.h"
#include "captdns.h"
/*#include "cert.h"
#include "private_key.h"*/

#undef LWIP_MDNS


#ifdef NOSSL
sint8 espconn_secure_connect(struct espconn *espconn){return 0;}
sint8 espconn_secure_disconnect(struct espconn *espconn){return 0;}
sint8 espconn_secure_sent(struct espconn *espconn, uint8 *psent, uint16 length){return 0;}
bool espconn_secure_set_size(uint8 level, uint16 size){return 0;}
#endif

#ifdef WEBSERVER
#include "web-server.h"
#endif

#ifdef SYSLOG
#include "syslog.h"
#define NOTICE(format, ...) do {                                            \
  LOG_NOTICE(format, ## __VA_ARGS__ );                                      \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )
#else
#define NOTICE(format, ...) do {                                            \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )
#endif

#ifdef MEMLEAK_DEBUG
#include "mem.h"
bool ICACHE_FLASH_ATTR check_memleak_debug_enable(void)
{
    return MEMLEAK_DEBUG_ENABLE;
}
#endif

/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[] = {
  { "/", cgiMetaHome, NULL,0 }, // if password set : /welcome.html else /init.html
//  { "/meta.wav", cgiMetaWav, NULL ,0},
  { "/meta/sand", cgiMetaSend, NULL,0 },
  { "/meta/state", cgiMetaState, NULL,0 },
  { "/meta/fetch", cgiMetaFetch, NULL,0 },
  { "/meta/gettimers", cgiMetaGetTimers, NULL,0 },
  { "/meta/userpass", cgiMetaUserPass, NULL,0 },
  { "/meta/auth", cgiMetaAuth, "/welcome.html",0 },
  { "/meta/ssid", cgiMetaGetSSID, NULL,0 },
  { "/logout", cgiMetaLogout, "/",'a' },
  { "/meta/gpio", cgiMetaGpio, NULL ,0},
  { "/meta/signal", cgiMetaGetSignal, NULL,0 },
  { "/meta/dump", cgiMetaDump, NULL,0 },
  { "/menu", cgiMenu, NULL ,0},
  { "/flash/next", cgiGetFirmwareNext, NULL ,0},
  { "/flash/upload", cgiUploadFirmware, NULL ,0},
  { "/flash/reboot", cgiRebootFirmware, NULL ,0},

  { "/pgm/sync", cgiOptibootSync, NULL ,0},
  { "/pgm/upload", cgiOptibootData, NULL,0 },

  { "/pgmmega/sync", cgiMegaSync, NULL,0 },		// Start programming mode
  { "/pgmmega/upload", cgiMegaData, NULL ,0},		// Upload stuff
  { "/pgmmega/read/*", cgiMegaRead, NULL,0 },		// Download stuff (to verify)
  { "/pgmmega/fuse/*", cgiMegaFuse, NULL ,0},		// Read or write fuse
  { "/pgmmega/rebootmcu", cgiMegaRebootMCU, NULL ,0},	// Get out of programming mode

  { "/log/text", ajaxLog, NULL ,0},
  { "/log/dbg", ajaxLogDbg, NULL ,0},
  { "/log/reset", cgiReset, NULL ,0},
  { "/console/reset", ajaxConsoleReset, NULL ,0},
  { "/console/baud", ajaxConsoleBaud, NULL ,0},
  { "/console/fmt", ajaxConsoleFormat, NULL ,0},
  { "/console/text", ajaxConsole, NULL ,0},
  { "/console/send", ajaxConsoleSend, NULL ,0},
  { "/user.html", cgiEspFsHook, NULL ,'a'},
  { "/wifi.html", cgiEspFsHook, NULL ,'a'},
  //Enable the line below to protect the WiFi configuration with an username/password combo.
//{"/wifi/*", authBasic, metaAuth,0},
  { "/wifi", cgiRedirect, "/wifi/wifi.html" ,0},
  { "/wifi/", cgiRedirect, "/wifi/wifi.html" ,0},
  { "/wifi/info", cgiWifiInfo, NULL,0},
  { "/wifi/scan", cgiWiFiScan, NULL ,0},
  { "/wifi/connect", cgiWiFiConnect, NULL ,0},
  { "/wifi/connstatus", cgiWiFiConnStatus, NULL ,0},
  { "/wifi/setmode", cgiWiFiSetMode, NULL,0 },
  { "/wifi/special", cgiWiFiSpecial, NULL,0 },
  { "/wifi/apinfo", cgiApSettingsInfo, NULL ,0},
  { "/wifi/apchange", cgiApSettingsChange, NULL,0 },
  { "/system/info", cgiSystemInfo, NULL ,0},
//  { "/system/update", cgiSystemSet, NULL ,0},
  { "/services/info", cgiServicesInfo, NULL ,0},
  { "/services/update", cgiServicesSet, NULL ,0},
//  { "/pins", cgiPins, NULL ,0},
#ifdef MQTT
  { "/mqtt", cgiMqtt, NULL },
#endif
#ifdef WEBSERVER
  { "/web-server/upload", cgiWebServerSetupUpload, NULL ,0},
  { "/web-server/list", cgiWebServerList, NULL ,0},
  { "*.json", WEB_CgiJsonHook, NULL,0 }, //Catch-all cgi JSON queries
#endif
  { "*", cgiEspFsHook, NULL ,0}, //Catch-all cgi function for the filesystem
  { NULL, NULL, NULL ,0}
};

#ifdef SHOW_HEAP_USE
static ETSTimer prHeapTimer;
static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg) {
  os_printf("Heap: %ld\n", (unsigned long)system_get_free_heap_size());
}
#endif

# define VERS_STR_STR(V) #V
# define VERS_STR(V) VERS_STR_STR(V)
char* esp_link_version = VERS_STR(VERSION);

// address of espfs binary blob
extern uint32_t _binary_espfs_img_start;

extern void app_init(void);
extern void mqtt_client_init(void);

void ICACHE_FLASH_ATTR
user_rf_pre_init(void) {
  //default is enabled
  system_set_os_print(DEBUG_SDK);
}

/* user_rf_cal_sector_set is a required function that is called by the SDK to get a flash
 * sector number where it can store RF calibration data. This was introduced with SDK 1.5.4.1
 * and is necessary because Espressif ran out of pre-reserved flash sectors. Ooops...  */
/*uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void) {
  uint32_t sect = 0;
  switch (system_get_flash_size_map()) {
  case FLASH_SIZE_4M_MAP_256_256: // 512KB
    sect = 128 - 10; // 0x76000
    break;
  default:
    sect = 128; // 0x80000
  }
  return sect;
}
*/
// Main routine to initialize meta-id.
void ICACHE_FLASH_ATTR
user_init(void) {
bool restoreOk;
  // uncomment the following three lines to see flash config messages for troubleshooting
  //uart_init(115200, 115200);
  //logInit();
  //os_delay_us(100000L);
	os_printf("SDK version:%s\n", system_get_sdk_version());
	system_print_meminfo();
  restoreOk = configRestore();
  // Init gpio pin registers
  gpio_init();
  gpio_output_set(0, 0, 0, (1<<15)); // some people tie it to GND, gotta ensure it's disabled
/*
  restoreOk=meta_init_gpio();
  // get the flash config so we know how to init things
   if (restoreOk)
		restoreOk= configRestore();
	else{
		configWipe(); // uncomment to reset the config for testing purposes	
		wifiConfigWipe();
	}*/
  // init UART
  uart_init(CALC_UARTMODE(flashConfig.data_bits, flashConfig.parity, flashConfig.stop_bits),
            flashConfig.baud_rate, 115200);
  logInit(); // must come after init of uart
  // Say hello (leave some time to cause break in TX after boot loader's msg
  os_delay_us(10000L);
  os_printf("\n\n** %s\n", esp_link_version);
  os_printf("Flash config restore %s\n", restoreOk ? "ok" : "*FAILED*");
  // Status LEDs
//  statusInit();
  serledInit();
/*
espconn_secure_set_default_certificate(default_certificate,default_certificate_len);
espconn_secure_set_default_private_key(default_private_key,default_private_key_len);
*/
  // Wifi
  wifiInit();
  // init the flash filesystem with the html stuff
  espFsInit(espLinkCtx, &_binary_espfs_img_start, ESPFS_MEMORY);

  //EspFsInitResult res = espFsInit(&_binary_espfs_img_start);
  //os_printf("espFsInit %s\n", res?"ERR":"ok");
  // mount the http handlers
  httpdInit(builtInUrls, flashConfig.hostname, 80);
#ifdef WEBSERVER
  WEB_Init();
#endif

  // init the wifi-serial transparent bridge (port 23)
  serbridgeInit(23, 2323);
  uart_add_recv_cb(&serbridgeUartCb);
  captdnsInit();
#ifdef SHOW_HEAP_USE
  os_timer_disarm(&prHeapTimer);
  os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
  os_timer_arm(&prHeapTimer, 10 * 1000, 1);
#endif

  struct rst_info *rst_info = system_get_rst_info();
  NOTICE("Reset cause: %d=%s", rst_info->reason, rst_codes[rst_info->reason]);
  NOTICE("exccause=%d epc1=0x%x epc2=0x%x epc3=0x%x excvaddr=0x%x depc=0x%x",
    rst_info->exccause, rst_info->epc1, rst_info->epc2, rst_info->epc3,
    rst_info->excvaddr, rst_info->depc);
  uint32_t fid = spi_flash_get_id();
  NOTICE("Flash map %s, manuf 0x%02X chip 0x%04X", flash_maps[system_get_flash_size_map()],
      fid & 0xff, (fid&0xff00)|((fid>>16)&0xff));
  NOTICE("** %s: ready, heap=%ld", esp_link_version, (unsigned long)system_get_free_heap_size());

  // Init SNTP service
  cgiServicesSNTPInit();
#ifdef MQTT
  if (flashConfig.mqtt_enable) {
    NOTICE("initializing MQTT");
    mqtt_client_init();
  }
#endif
  NOTICE("initializing user application");
  //app_init();
  NOTICE("Waiting for work to do...");
#ifdef MEMLEAK_DEBUG
  system_show_malloc();
#endif
}
