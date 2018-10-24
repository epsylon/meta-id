/*
*
* this file contains additions to the original esp-link firmware
*/


/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * ikujam <ikujam@ikujam.org> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include <esp8266.h>
#include "cgiwifi.h"
#include "cgi.h"
#include "config.h"
#include "sntp.h"
#include "cgimeta.h"
#include "cgiwifi.h"
#include "httpclient.h"
#include "hash.h"
#include "pwm.h"
#include "mqtt.h"
#include "mqtt_client.h"
#ifdef SYSLOG
#include "syslog.h"
#endif

#if 0
#define DBG(format, ...) do { os_printf(format, ## __VA_ARGS__); } while(0)
#else
#define DBG(format, ...) do { ; } while(0)
#endif

#define PWM_0_OUT_IO_MUX  PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM  12
#define PWM_0_OUT_IO_FUNC FUNC_GPIO12
#define PWM_CHANNEL 1

static uint32 systime, rtctime;
extern MQTT_Client mqttClient;

static char *connStatuses[] = { "idle", "connecting", "wrong password", "AP not found",
                         "failed", "got IP address" };

// translate an integer into "base 64" : [0-9a-zA-Z_-] -> 64 characters
char ICACHE_FLASH_ATTR translate(short i){
  if(i<10) return i+'0';
  if(i<36) return i-10+'a';
  if(i<62) return i-36+'A';
  if(i==62) return '_';
  if(i==63) return '-';
  return '?';
}

// generate ssid from the last 6 hex charcters of mac
// 6 hex <=> 2**(4*6) = 64**4 <=> 4 "base64"

void ICACHE_FLASH_ATTR metaSSID(char* output){
  char input[6];
	wifi_get_macaddr(1, (uint8*)input);
	for(int i=0;i<3;i++){
	input[2*i]=input[i+3]/16;
	input[2*i+1]=input[i+3]%16;
	}
  output[0]=translate(input[0]*4 + input[1]/4);
  output[1]=translate((input[1]%4)*16 + input[2]);
  output[2]=translate(input[3]*4 + input[4]/4);
  output[3]=translate((input[4]%4)*16 + input[5]);
}

int ICACHE_FLASH_ATTR ICACHE_FLASH_ATTR cgiMetaDump(HttpdConnData *connData) {
  char buff[1024];

  if (connData->conn == NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.

  struct rst_info *rst_info = system_get_rst_info();

  os_sprintf(buff,
    "{ "
      "\"name\": \"%s\", "
      "\"reset cause\": \"%d=%s\", "
      "\"pass\": \"%s\", "
      "\"description\": \"%s\","
      "\"voltage\": \"%d\""
    " }",
    flashConfig.hostname,
    rst_info->reason,
    rst_codes[rst_info->reason],
    flashConfig.user_pass,
    flashConfig.sys_descr,
    system_adc_read()
    );
	pwm_set_duty(255, 0);
	pwm_start();
	
  jsonHeader(connData, 200);
  httpdSend(connData, buff, -1);
  return HTTPD_CGI_DONE;
}
int metaCheckHash (int32 hash){
	int32 flashhash;
	flashhash=SuperFastHash(flashConfig.user_pass);
	return flashhash==hash;
}

int ICACHE_FLASH_ATTR cgiMetaUserPass(HttpdConnData *connData) {
  int8 pl;
  char passwd[USER_PASS_LENGTH];
	memset(passwd,0,USER_PASS_LENGTH);
  if (connData->conn==NULL) 
	return HTTPD_CGI_DONE;
	pl = 0;
	pl|=getStringArg(connData, "passwd", passwd,USER_PASS_LENGTH);
	if (pl<0){
		DBG("META: setting UserPass failed !\n");
		return HTTPD_CGI_DONE;
	}
	DBG("META: setting UserPass to |%s|\n",flashConfig.user_pass);
	os_sprintf(flashConfig.user_pass,passwd);
	if (configSave()) {
	int32 hash;
	hash= SuperFastHash(flashConfig.user_pass);
	return httpdSetCookie(connData,"/welcome.html",hash);
  }
  else {
    httpdStartResponse(connData, 500);
    httpdEndHeaders(connData);
    httpdSend(connData, "Failed to save config", -1);
  }
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiMetaLogout(HttpdConnData *connData) {
  if (connData->conn==NULL) 
	return HTTPD_CGI_DONE;
	httpdSetCookie(connData,"/welcome.html",0);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiMetaCheckAuth(HttpdConnData *connData) {
	int32 hash;
  if (connData->conn==NULL) 
	return HTTPD_CGI_DONE;
	hash=SuperFastHash(flashConfig.user_pass);
	DBG("META: checkMetaCheckAuth |%d| - |%d|\n",hash,connData->hash);
	if(hash==connData->hash)
		httpdRedirect(connData,(char*)connData->cgiArg);
	else
		httpdForbidden(connData) ;
	
  return HTTPD_CGI_DONE;

}

int ICACHE_FLASH_ATTR cgiMetaCheckAuthCgi(HttpdConnData *connData) {
	int32 hash;
	int r;
  if (connData->conn==NULL) 
	return HTTPD_CGI_DONE;
	hash=SuperFastHash(flashConfig.user_pass);
	DBG("META: checkAuthCgi with |%d| - |%d|\n",hash,connData->hash);
	if(hash==connData->hash){
		connData->cgi=connData->cgiArg;
		connData->cgiArg=NULL;
    r = connData->cgi(connData);
    if (r == HTTPD_CGI_MORE) {
      //Yep, it's happy to do so and has more data to send.
      httpdFlush(connData);
      return r;
    }
    else if (r == HTTPD_CGI_DONE) {
      //Yep, it's happy to do so and already is done sending data.
      httpdFlush(connData);
      connData->cgi = NULL; //mark for destruction.
      if (connData->post) connData->post->len = 0; // skip any remaining receives
      return r;
    }
	}
	else
		httpdForbidden(connData) ;
	
  return HTTPD_CGI_DONE;

}

int ICACHE_FLASH_ATTR cgiMetaAuth(HttpdConnData *connData) {
	int32 hash, flashhash;
	int8 pl;
	char passwd[USER_PASS_LENGTH];
	char buff[1024];
	memset(passwd,0,USER_PASS_LENGTH);
	if (connData->conn==NULL) 
		return HTTPD_CGI_DONE;
    flashhash=SuperFastHash(flashConfig.user_pass);
	pl = 0;
	pl|=getStringArg(connData, "passwd", passwd, USER_PASS_LENGTH);
	DBG("META: auth...%d - %s\n",pl,passwd);
	if (pl<=0){
		if(connData->hash==flashhash){
			jsonHeader(connData, 200);
			os_sprintf(buff,"{\"state\":1,\"msg\":\"Login success\"}");
			httpdSend(connData, buff, -1);
		}
		else{
			jsonHeader(connData, 401);
			os_sprintf(buff,"{\"state\":0,\"msg\":\"Login refused\"}");
			httpdSend(connData, buff, -1);
		}
		return HTTPD_CGI_DONE;
	}
	hash= SuperFastHash(passwd);
	DBG("META: auth with |%d| - |%d|[%s]\n",hash,flashhash,passwd);
	if (hash==flashhash){
//			return httpdSetCookie(connData, "/meta/auth", hash) ;
		if(connData->hash==0){
			httpdSendAuthCookie(connData,hash);
		}
		else{
			jsonHeader(connData, 200);
			os_sprintf(buff,"{\"state\":2,\"msg\":\"Login success\"}");
			httpdSend(connData, buff, -1);
		}
	}
	else{
		int l = os_sprintf(buff, "HTTP/1.0 401 Forbidden\r\nServer: meta-id\r\nConnection: close\r\n"
      "Set-Cookie: h=0; path=/; expires=Mon, 1 Jan 1979 23:42:01 GMT; max-age: 3600; HttpOnly"
      "\r\n\r\n{\"state\":0,\"msg\":\"Login refused\"}\r\n");
		httpdSend(connData, buff, l);
	}
	return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiMetaHome(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
//	FlashConfig.user_pass
	if(flashConfig.user_pass[0])
		httpdRedirect(connData, "/welcome.html") ;
	else
		httpdRedirect(connData, "/init.html") ;
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiMetaGetSignal(HttpdConnData *connData) {
  char buff[1024];
  int len;
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
  len = os_sprintf(buff, "{ \"signal\":%d}",	wifiSignalStrength(-1));
  jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;
}

void metaHttpCallback(char * response_body, int http_status, char * response_headers, int body_size)
{
	os_printf("http_status=%d", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		os_printf(" strlen(headers)=%d", strlen(response_headers));
		os_printf(" body_size=%d\n", body_size);
		os_printf("body=%s<EOF>\n", response_body);
		os_printf("duration %d / rtc : %d\n",system_get_time()-systime,system_get_rtc_time()-rtctime);
	}
}


int ICACHE_FLASH_ATTR cgiMetaSend(HttpdConnData *connData) {
  char buff[256];
  char msg[140];
  int len;
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
  len = 0;
  len|=getStringArg(connData, "msg", msg, 128);
  len = os_sprintf(buff, "http://x.ikujam.org/mqtt/submit.php?code=abcd&signal=%d&msg=%s",	wifiSignalStrength(-1),msg);
  metaSSID(buff+41);
  http_get(buff, buff, metaHttpCallback);
  jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  systime=system_get_time();
  rtctime=system_get_rtc_time();
  len=os_strlen(msg);
  msg[len]=',';
  metaSSID(msg+os_strlen(msg)+1);
  msg[len+5]=0;
//  wpXmlRpcSend(msg);
  return HTTPD_CGI_DONE;
}


int ICACHE_FLASH_ATTR cgiMetaFetch(HttpdConnData *connData) {
  char buff[1024];
  int len;
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
 
  len = os_sprintf(buff, "http://x.ikujam.org/random.bin");
 http_get(buff, "", metaHttpCallback);
 jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  systime=system_get_time();
  rtctime=system_get_rtc_time();
  return HTTPD_CGI_DONE;
}


int ICACHE_FLASH_ATTR cgiMetaGetGpio(HttpdConnData *connData) {
  char buff[1024];
  int len;
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
  len = os_sprintf(buff, "{ ");
  for (int i=0; i<15; i++) {
	len += os_sprintf(buff+len, "\"meta-gpio-%02d\":%d, ",i,GPIO_INPUT_GET(GPIO_ID_PIN(i))+1);
  }
	len += os_sprintf(buff+len, "\"meta-gpio-%02d\":%d ",15,GPIO_INPUT_GET(GPIO_ID_PIN(15))+1);
	len += os_sprintf(buff+len,"}");
  jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;

}
	
int ICACHE_FLASH_ATTR cgiMetaSetGpio(HttpdConnData *connData) {
// set input/output high/output low

  char buff[1024];
  int len;
  int8 ok = 0; // error indicator
  int8 num;
  int8 out;
  ok |= getInt8Arg(connData, "num", &num);
  ok |= getInt8Arg(connData, "v", &out);
	if (!ok){
		len = os_sprintf(buff, "invalid parameter");
		jsonHeader(connData, 200);
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}
	if (num < 0 || num > 15){
		len = os_sprintf(buff, "ko: invalid number %d ",num);
		jsonHeader(connData, 200);
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}
	if (out < -2 || out > 2){
		len = os_sprintf(buff, "ko: invalid value %d ",out);
		jsonHeader(connData, 200);
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}
		switch(num){
			case 0:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO0); break;
			case 1:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO1); break;
			case 2:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO2); break;
			case 3:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO3); break;
			case 4:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO4); break;
			case 5:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO5); break;
//			case 6:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO6); break;
//			case 7:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO7); break;
//			case 8:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO8); break;
			case 9:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO9); break;
			case 10:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO10); break;
//			case 11:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO11); break;
			case 12:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12); break;
			case 13:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO13); break;
			case 14:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO14); break;
			case 15:PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO15); break;
			default:
				len = os_sprintf(buff, "ko: unhandled number %d ",num);
				jsonHeader(connData, 200);
				httpdSend(connData, buff, len);
				return HTTPD_CGI_DONE;
	}

  len = os_sprintf(buff, "%d",ok);
  if(out == -1){
	GPIO_OUTPUT_SET(GPIO_ID_PIN(num), 1); 
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDI_U); 
	  }
  if(out == -2){
	GPIO_OUTPUT_SET(GPIO_ID_PIN(num), 0); 
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDI_U); 
	  }
  if(out == 1){
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(num)); 
	  }
  jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;

}



int ICACHE_FLASH_ATTR cgiMetaGpio(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.
  if (connData->requestType == HTTPD_METHOD_GET) {
    return cgiMetaGetGpio(connData);
  } else if (connData->requestType == HTTPD_METHOD_POST) {
    return cgiMetaSetGpio(connData);
  } else {
    jsonHeader(connData, 404);
    return HTTPD_CGI_DONE;
  }
}

char gethex(char c){
  if (c-'0'<10 && c-'0'>=0)return c-'0';
  if (c>='a' && c<='z') return 10+c-'a';
  if (c>='A' && c<='Z') return 10+c-'A';
  printf ("gethex: invalid input : %d\n",c);
  return '?';
}

int ICACHE_FLASH_ATTR cgiMetaGetSSID(HttpdConnData *connData) {
  char buff[5];
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
    metaSSID(buff);
	buff[4]=0;
jsonHeader(connData, 200);
  httpdSend(connData, buff, 4);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiMetaGetTimers(HttpdConnData *connData) {
  char buff[1024];
  int len;
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
	len=os_sprintf(buff,"{\"sys\":%d;\"rtc\":%d}",systime,rtctime);
jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;
}


/*
int ICACHE_FLASH_ATTR meta_init_gpio() {
	int r=1;
	if(GPIO_INPUT_GET(GPIO_ID_PIN(0))==1){
		for(int i=0;i<42;i++){
			os_delay_us(64000);
			if(GPIO_INPUT_GET(GPIO_ID_PIN(0))!=1){
				i=42;
				r=0;
				GPIO_OUTPUT_SET(GPIO_ID_PIN(13),i%2);
			}
		}
	}
	return r;
}*/
/**
 * -1 : unknown, waiting for authentication
 *  0 : login box // if no wifi config and no auth
 *  1 : login success // if auth ok
 *  2 : wifi config // if no wifi connection and on request
 *  3 : connecting //                                                                                                                                                                                                                                                                                                                                                                                                 
 *  4 : connected
 * "idle", "connecting", "wrong password", "AP not found","failed", "got IP address"
 **/
int ICACHE_FLASH_ATTR cgiMetaState(HttpdConnData *connData) {
	char buff[512];
	char *msg;
	int auth=0;
    int state = wifi_station_get_connect_status();
	int32 hash=SuperFastHash(flashConfig.user_pass);
	int len;
	DBG("META: checkMetaCheckAuth |%d| - |%d|\n",hash,connData->hash);
	if (hash==connData->hash)
		auth=1;
    if (state >= 0 && state < sizeof(connStatuses)) {
		msg = connStatuses[state];
		switch(state){
			case 0:
				state=-1;
				break;
			case 1:
				state=3;
				break;
			case 2:
				break;
			case 3:
			case 4:
				if(auth)
					state=2;
				else
					state=0;
				break;
			case 5:
				state=4;
				break;
		}
	}
	else{
		if(auth){
			state=2;
			msg="wifi config";
		}
		else{
			state=-1;
			msg="unknown";
		}
	}
	pwm_set_duty(10, 0);
	pwm_start();
	len=os_sprintf(buff,"{\"state\":%d,\"auth\":%d,\"msg\":\"%s\"}",state,auth,msg);
	jsonHeader(connData, 200);
	httpdSend(connData, buff, len);
	return HTTPD_CGI_DONE;
}



int ICACHE_FLASH_ATTR cgiMetaMqtt(HttpdConnData *connData) {
  int8_t mqtt_en_chg = getBoolArg(connData, "mqtt-enable",
      &flashConfig.mqtt_enable);
  // if server setting changed, we need to "make it so"
 if (mqtt_en_chg > 0) {
    DBG("MQTT server enable=%d changed\n", flashConfig.mqtt_enable);
    if (flashConfig.mqtt_enable && strlen(flashConfig.mqtt_host) > 0) {
      MQTT_Free(&mqttClient); // safe even if not connected
      mqtt_client_init();
      MQTT_Reconnect(&mqttClient);
    } else {
      MQTT_Disconnect(&mqttClient);
      MQTT_Free(&mqttClient); // safe even if not connected
    }
  }
  // no action required if mqtt status settings change, they just get picked up at the
  // next status tick
  if (getBoolArg(connData, "mqtt-status-enable", &flashConfig.mqtt_status_enable) < 0)
    return HTTPD_CGI_DONE;
  if (getStringArg(connData, "mqtt-status-topic",
        flashConfig.mqtt_status_topic, sizeof(flashConfig.mqtt_status_topic)) < 0)
    return HTTPD_CGI_DONE;

  DBG("Saving config\n");

  if (configSave()) {
    httpdStartResponse(connData, 200);
    httpdEndHeaders(connData);
  } else {
    httpdStartResponse(connData, 500);
    httpdEndHeaders(connData);
    httpdSend(connData, "Failed to save config", -1);
  }
  return HTTPD_CGI_DONE;
}


