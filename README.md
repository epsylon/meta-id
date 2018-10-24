META-ID: Wifi-Serial Bridge w/REST&MQTT
========================================

http://www.meta-id.info/

The meta-id firmware connects a micro-controller to the internet using an ESP8266 Wifi module.

it is based on the awesome esp-link firmware from jeelabs : https://github.com/jeelabs/esp-link

THIS PROJECT IS WORK IN PROGRESS
================================

It implements a number of features:

- transparent bridge between Wifi and serial, useful for debugging or inputting into a uC
- flash-programming attached Arduino/AVR microcontrollers and
  LPC800-series and other ARM microcontrollers via Wifi
- built-in stk500v1 programmer for AVR uC's: program using HTTP upload of hex file
- outbound REST HTTP requests from the attached micro-controller to the internet
- MQTT client pub/sub from the attached micro-controller to the internet
- serve custom web pages containing data that is dynamically pulled from the attached uC and
  that contain buttons and fields that are transmitted to the attached uC (feature not
  fully ready yet)

The firmware includes a tiny HTTP server based on
[esphttpd](http://www.esp8266.com/viewforum.php?f=34)
with a simple web interface, many thanks to Jeroen Domburg for making it available!
The REST and MQTT functionality are loosely based on [espduino](https://github.com/tuanpmt/espduino)
but significantly rewritten and no longer protocol compatible, thanks to tuanpmt for the
inspiration!
