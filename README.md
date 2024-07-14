# ESP32SerialNTP-BLE-Clock
Time feed ATMEGA B/W Word clock with NTP time with a ESP32-C3-supermini, ESP32-S3-Zero, Arduino Nano ESP32 orESP-C3-12F-Kit. 
![ESP32ToWordClock](https://github.com/ednieuw/ESP32SerialNTP-BLE-Clock/assets/12166816/ac6b3e92-fde4-49cc-915b-91af3d018617)

The word clock with white 2835 or 3528 LEDs runs on an ATMEGA 328 and keeps time in a DS3231 Module.
See here: https://github.com/ednieuw/Woordklok-witte-LEDs.

This word clock can receive its time with a DCF77 receiver. But this will not work in a clock with brass copper or corten steel word plates, a bad position or interfering radio waves in the neighbourhood.<br>
![woordklokCIMG2963](https://github.com/ednieuw/ESP32SerialNTP-BLE-Clock/assets/12166816/ed78be12-9529-405e-b355-75b00a09bf1c)

The ESP32 used will replace the Bluetooth module on the word clock PCB. The ESP32 has Bluetooth and WIFI connectivity adding two new functionalities, namely a web browser page and a NTP time clock that receive time from the internet.
The time is send regulary over the serial port pins in the format Thhmmss to the word clock keeping its time and day light savings correct. 

![image](https://github.com/ednieuw/ESP32SerialNTP-BLE-Clock/assets/12166816/a7aacb75-6bb4-4673-abc5-d958b976eb5a)
ESP32-C3-supermini --- ESp32-S3-Zero --- Arduino Nano ESP32 --- ESP-C3-!2F-Kit 

This code can be used with an ESP32 C3 and S3 and probably other boards.

With a #define in the code a board can be selected before compiling the code for that board.<br>

```
//#define ESP32C3_SUPERMINI   // ESP32-C3-Supermini
//#define ESP32S3_ZERO        // ESP32-S3-Zero
//#define ESP32C3_DEV         // ESP-C3-12F-Kit 
//#define ESP32S3_DEV         // ESP32-S3
#define NanoESP32             // Arduino Nano ESP32
``` 
Differences between the code for the several boards are for the LEDs on the board. These LEDs are not essential and the coding can be  deleted from the code making the software suitable for many boards. These two subroutines control the LEDs.
```
void UpdateStatusLEDs(int Toggle)
void SetStatusLED(int WW, int CW, int Re, int Gr, int Bl)
```
Version V010 and higher will be suited for the Arduino nano ESP32. A board that will probably be avaiable for many years after this year 2024 and my choice to develop further with.

I made some cables to connect the ESP32-super mini and the ESP32-S3 Zero to the Word clock PCB.
The ESP32-Super mini is difficult to solder pins to it without destroying the unit. The tiny electronic parts near the pin connections melt.<br> 
When you solder the first 5 pins on the ESP32-C3 super mini to 5V, GND, 3V3, GPIO4 and GPIO3 from the bottom there are no tiny parts you can destroy while welding.<br>
After several prototypes I was most happy with a M-F Dupont cable and five 90 degree pins soldered to the board. 

The ESP32-S3 Zero from Waveshare with pins welded to it is a good alternative.<br>
The ESP32-C3-12F is a robust board I have lying around and has probably a stronger WIFI and BLE.

![image](https://github.com/ednieuw/ESP32SerialNTP-BLE-Clock/assets/12166816/0785be7a-b2b1-4e4f-80d8-a79bf15664c5)<br>
Several connector options between the Word clock board and the ESP32-C3 and -S3 mini boards

![image](https://github.com/ednieuw/ESP32SerialNTP-BLE-Clock/assets/12166816/071893be-9830-49bd-8023-2304a4e0f016)<br>
Arduino Nano ESP32 connected via pins 5V, GND, A6 and A7 to the Bluetooth connection on the Word clock PCB.

**A better approach is to connect the Vin of the ESP32 to the 12V input of the PCB. The L7805 becomes very hot when it has to supply the power for the ESP32.**

# How to use
- Select the board you use in the source code. (Remove the // before the board that is used and select only ONE board).
- The RX and TX pins that are connected to the Word clock and defined here. 
```
                         #ifdef ESP32C3_SUPERMINI
#define LEDPIN 8
#define RX1PIN 4
#define TX1PIN 3 
                        #endif //ESP32C3_SUPERMINI
                        #ifdef NANOESP32 
#define RX1PIN A6  // 13   Connect to Rx
#define TX1PIN A7  // 14   Connect to Tx 
                        #endif //NANOESP32
```
- Connect the board, check the compile options as noted in the top of the source code, compile for the proper board and upload the code
- Open the serial terminal and **send '?i'**<br>
  (The ESP32 menu only opens when the **command is preceded with an ? character**. <br>
  Without the questionmark '?' character the command will be send to the Word clock menu)<br>
  An Information menu will be displayed.<br>
- Enter your router SSID name preceded with an 'a'  ( aSSIDNAME )
- Enter the password of the router preceded with a 'b' ( bPASSWORD )
- Enter a name for your BLE station preceded with a 'c' ( cBLENAME ) 
- Send a @ to restart the ESP32.
  If all is well the proper time will be printed after a restart and a IP-address other than 0.0.0.0 will be printed in the info menu.
 
The following will be printed in the serial terminal of the connected PC via the USB port or in the serial app on your phone.
Search in IOS app store for [BLEserial](https://apps.apple.com/nl/app/bleserial-nrf/id1632235163) or [BLEserial Pro](https://apps.apple.com/nl/app/ble-serial-pro/id1632245655)
and for Android [Bluetooth terminal](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal&pli=1) 

```
Serial started
Mem.Checksum = 25065
Setting loaded
BLE started
Starting WIFI
Web page started
IP Address: 192.168.178.156
NTP is On
___________________________________
?A SSID B Password C BLE beacon name
?D Date (D15012021) T Time (T132145)
?E Timezone  (E<-02>2 or E<+01>-1)
?I To print this Info menu
?P Status LED toggle On/Off
?R Reset settings @ = Reset MCU
?W=WIFI ? X=NTP& ?Y=BLE  ?Z=Fast BLE
Ed Nieuwenhuys Jun 2024
___________________________________
SSID: FRITZ!BoxEd
BLE name: ZeroS3TimeFeed
IP-address: 192.168.178.156  (/update)
Timezone:CET-1CEST,M3.5.0,M10.5.0/3
WIFI=On NTP=On BLE=On FastBLE=On
Software: ESP32C3S3_SerialClockV005.ino
00/01/1900 00:00:00 
___________________________________
16/06/2024 15:17:44 
```
- Connect the RX, TX, the 5V and GND pins between the ESP32 and the Bluetooth connection pins.
if an USB-cable is connected to the ESP32 the clock will start working because the 5V power line will feed the ATMEGA chip<br>
The seconds LED on the Wordclock PCB will start flashing. The LEDs will ofcourse not light up. They need the 12V power supply.

- Check in a serial terminal time is send every six minutes to the word clock. 
  If it does not work TX and RX are probably reversed.

```
___________________________________
SSID: FRITZ!BoxEd
BLE name: ZeroS3TimeFeed
IP-address: 192.168.178.156 (/update)
Timezone:CET-1CEST,M3.5.0,M10.5.0/3
WIFI=On NTP=On BLE=On FastBLE=On
Software: ESP32C3S3_SerialClockV005.ino
00/01/1900 00:00:00 
___________________________________
16/06/2024 15:17:44 T151744
Het is kwart over drie 15:17:44
15:17:44
Het is kwart over drie Sensor:907 Min:905 Max:905 Out: 71=27% Temp:19C 15:18:00
Het is kwart over drie 15:18:00 16-06-2024
Sensor:907 Min:905 Max:905 Out: 71=27% Temp:20C 15:18:30
Sensor:904 Min:904 Max:905 Out: 71=27% Temp:20C 15:19:00
Het is kwart over vier 15:19:00 16-06-2024
Sensor:907 Min:904 Max:905 Out: 71=27% Temp:20C 15:19:30
Sensor:907 Min:904 Max:905 Out: 71=27% Temp:20C 15:20:00
Het is kwart over vier 15:20:00 16-06-2024
Sensor:908 Min:904 Max:906 Out: 71=27% Temp:20C 15:20:30
T152100
Het is is tien voor half vier 15:21:00
15:21:00
Het is is tien voor half vier Sensor:907 Min:904 Max:906 Out: 71=27% Temp:20C 15:21:00
Het is is tien voor half vier 15:21:00 16-06-2024
Sensor:902 Min:902 Max:906 Out: 71=27% Temp:20C 15:21:30
Sensor:898 Min:898 Max:906 Out: 71=27% Temp:20C 15:22:00
Het is is tien voor half vier 15:22:00 16-06-2024
Sensor:893 Min:893 Max:906 Out: 71=27% Temp:20C 15:22:30
Sensor:886 Min:886 Max:906 Out: 71=27% Temp:20C 15:23:00
Het is is tien voor half vier 15:23:00 16-06-2024
Sensor:880 Min:880 Max:906 Out: 70=27% Temp:20C 15:23:30
Sensor:877 Min:877 Max:906 Out: 70=27% Temp:20C 15:24:00
Het is is tien voor half vier 15:24:00 16-06-2024
Sensor:871 Min:871 Max:906 Out: 70=27% Temp:20C 15:24:30
Sensor:861 Min:861 Max:906 Out: 70=27% Temp:21C 15:25:00
Het is vijf voor half vier 15:25:00 16-06-2024
Sensor:852 Min:852 Max:906 Out: 69=27% Temp:21C 15:25:30
Sensor:842 Min:842 Max:906 Out: 69=27% Temp:21C 15:26:00
Het is vijf voor half vier 15:26:00 16-06-2024
Sensor:832 Min:832 Max:906 Out: 68=26% Temp:21C 15:26:30
T152700
Het is vijf voor half vier 15:27:00
15:27:00
```


 
