#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_
#include <c_types.h>
#ifdef __WIN32__
#include <_mingw.h>
#endif

#undef SHOW_HEAP_USE
#undef DEBUGIP
#undef SDK_DBG

#undef CMD_DBG
#undef ESPFS_DBG
#undef CGI_DBG
#undef CGIFLASH_DBG
#undef CGIMQTT_DBG
#undef CGIPINS_DBG
#undef CGIWIFI_DBG
#undef CONFIG_DBG
#undef LOG_DBG
#undef STATUS_DBG
#undef HTTPD_DBG
#undef MQTT_DBG
#undef MQTTCMD_DBG
#undef MQTTCLIENT_DBG
#undef PKTBUF_DBG
#undef REST_DBG
#undef RESTCMD_DBG
#undef SERBR_DBG
#undef SERLED_DBG
#undef SLIP_DBG
#undef UART_DBG
#undef MDNS_DBG
#undef OPTIBOOT_DBG
#undef SYSLOG_DBG
#undef CGISERVICES_DBG

// If defined, the default hostname for DHCP will include the chip ID to make it unique
#undef CHIP_IN_HOSTNAME

extern char* esp_link_version;
extern uint8_t UTILS_StrToIP(const char* str, void *ip);

#endif
