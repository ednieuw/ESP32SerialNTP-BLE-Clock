/* 
Code to update time in ATMEGA328 B&W word clock.
--------------------------------------  
 Author .    : Ed Nieuwenhuys
 Changes V001: Derived from ESP32C3S3_WordClockV033 and ESP32Arduino_WordClockV029.ino
 Changes V005: ESP32-S3-Zero Working version 
 Changes V006: Cleaning up. Updated Webpage and SetStatusLeds
 Changes V007: ESP32C3_SUPERMINI Working version 
 Changes V008: ESP32C3_DEV = ESP-C3-12F-Kit Working version  
 Changes V009: Arduino Nano ESP32 Working version
 Changes V010: Updated web page. Serial ports defines per board.
               in ESP32 board a ESP32C3_SUPERMINI special board found Nologo ESP32C3 Super mini

If the serial communication to the word clock does noet work try switching the pin numbers
at #define RX1PIN  2  #define TX1PIN  1. Always confusing RX->TX and TX->RX 
 
How to compile: Install ESP32 boards

ESP32C3_SUPERMINI
Select Board: Nologo ESP32C3 Super mini
Compile and upload

ESP32S3_ZERO
Board: ESP32-S3 DEV module
Flash size 4MB ot 2MB if you have a cheaper 2MB board.
OTA will not work on 2MB boards
Patition Scheme: Default or change to Minimal SPIFFS (1.9Mb with OTA, 190kB SPIFFS) if there is a memoty shortage
USB-CDC on boot: ENABLED


How to compile for Arduino Nano ESP32: 
Install Arduino Nano ESP32 boards
Board: Arduino Nano ESP32
Partition Scheme: With FAT
Pin Numbering: By GPIO number (legacy)

*/
// =============================================================================================================================
#define ESP32C3_SUPERMINI   // ESP32-C3-Supermini
//#define ESP32S3_ZERO        // ESP32-S3-Zero
//#define ESP32C3_DEV         // ESP-C3-12F-Kit 
//#define ESP32S3_DEV         // ESP32-S3 Not yet implemented
//#define NANOESP32           // Arduino Nano ESP32

//--------------------------------------------                                                //
// ESP32-C3 Includes defines and initialisations
//--------------------------------------------

#include <NimBLEDevice.h>      // For BLE communication  https://github.com/h2zero/NimBLE-Arduino 1.4.1
#include <ESPNtpClient.h>      // https://github.com/gmag11/ESPNtpClient
#include <WiFi.h>              // Used for web page 
#include <AsyncTCP.h>          // Used for webpage   https://github.com/me-no-dev/ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // Used for webpage   https://github.com/me-no-dev/ESPAsyncWebServer
#include <ElegantOTA.h>        // Used for OTA
#include <Preferences.h>
#include <Adafruit_NeoPixel.h> 
//--------------------------------------------                                                //
// SPIFFS storage
//--------------------------------------------
Preferences FLASHSTOR;

// Pin Assignments
                        #ifdef ESP32C3_DEV
#define  REDPIN       3
#define  GREENPIN     4
#define  BLUEPIN      5
#define  COLD_WHITE  18
#define  WARM_WHITE  19
#define RX1PIN        2
#define TX1PIN        1   
                        #endif

                        #ifdef ESP32S3_ZERO
#define RX1PIN 5
#define TX1PIN 4                         
#define BRIGHTNESS 20 // Set BRIGHTNESS (max = 255)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, 21, NEO_RGB + NEO_KHZ800);                     // RGB LED on ESP32-S3-Zero GPIO21
                           #endif // ESP32S3_ZERO                     

                        #ifdef ESP32C3_SUPERMINI
#define LEDPIN 8
#define RX1PIN 4
#define TX1PIN 3 
                        #endif //ESP32C3_SUPERMINI
                        #ifdef NANOESP32 
#define RX1PIN 4
#define TX1PIN 3 
                        #endif //NANOESP32
//--------------------------------------------                                                //
// CLOCK initialysations
//--------------------------------------------                                 
static  uint32_t msTick;                                                                      // Number of millisecond ticks since we last incremented the second counter
byte    lastminute = 0, lasthour = 0, lastday = 0;
struct  tm timeinfo;                                                                          // storage of time 

//--------------------------------------------                                                //
// BLE   //#include <NimBLEDevice.h>
//--------------------------------------------
bool deviceConnected    = false;
bool oldDeviceConnected = false;
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
std::string ReceivedMessageBLE;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"                         // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

//--------------------------------------------                                                //
// WEBSERVER
//----------------------------------------
int WIFIConnected = 0;              // Is wIFI connected?
AsyncWebServer server(80);          // For OTA Over the air uploading
#include "Webpage.h"
//----------------------------------------
// NTP
//----------------------------------------
boolean syncEventTriggered = false;                                                           // True if a time even has been triggered
NTPEvent_t ntpEvent;                                                                          // Last triggered event
//--------------------------------------------                                                //
// Common
//----------------------------------------
 uint64_t Loopcounter = 0;
#define MAXTEXT 140
char sptext[MAXTEXT];                                                                         // For common print use 
bool SerialConnected = true;   
struct EEPROMstorage {                                                                        // Data storage in EEPROM to maintain them after power loss 
  byte DisplayChoice    = 0;                                                                  // Struct is used in all ednieuw project
  byte TurnOffLEDsAtHH  = 0;
  byte TurnOnLEDsAtHH   = 0;                                                                  // 
  byte LanguageChoice   = 0;
  byte LightReducer     = 0;
  int  LowerBrightness  = 0;
  int  UpperBrightness  = 0;
  int  NVRAMmem[24];                                                                          // LDR readings
  byte BLEOn            = 1;
  byte NTPOn            = 1;
  byte WIFIOn           = 1;  
  byte StatusLEDsOn     = 1;
  int  ReconnectWIFI    = 0;                                                                  // No of times WIFI reconnected 
  byte UseSDcard        = 0;
  byte WIFINoOfRestarts = 0;                                                                  // If 1 than resart MCU once
  byte UseDS3231        = 0;                                                                  // Use the DS3231 time module 
  byte LEDstrip         = 0;                                                                  // 0 = SK6812 LED strip. 1 = WS2812 LED strip
  byte ByteFuture3      = 0;                                                                  // For future use
  byte ByteFuture4      = 0;                                                                  // For future use
  byte UseBLELongString = 0;                                                                  // Send strings longer than 20 bytes per message. Possible in IOS app BLEserial Pro 
  uint32_t OwnColour    = 0;                                                                  // Self defined colour for clock display
  uint32_t DimmedLetter = 0;
  uint32_t BackGround   = 0;
  char Ssid[30];                                                                              // 
  char Password[40];                                                                          // 
  char BLEbroadcastName[30];                                                                  // Name of the BLE beacon
  char Timezone[50];
  int  Checksum        = 0;
}  Mem; 
//--------------------------------------------                                                //
// Menu
//0        1         2         3         4
//1234567890123456789012345678901234567890----  
 char menu[][40] = {
 "?A SSID B Password C BLE beacon name",
 "?D Date (D15012021) T Time (T132145)",
 "?E Timezone  (E<-02>2 or E<+01>-1)",
 "?I To print this Info menu",
 "?P Status LED toggle On/Off", 
 "?R Reset settings @ = Reset MCU",
 "?W=WIFI  ?X=NTP& ?Y=BLE  ?Z=Fast BLE", 
 "Ed Nieuwenhuys Jun 2024" };
 
//  -------------------------------------   End Definitions  ---------------------------------------

//--------------------------------------------                                                //
// ARDUINO Setup
//--------------------------------------------
void setup() 
{
 Serial.begin(9600);                                                                          // Setup the serial port to 9600 baud //
 Serial1.begin(9600, SERIAL_8N1, RX1PIN, TX1PIN);                                             // Serial1.begin(9600, SERIAL_8N1, RX1PIN, TX1PIN);                                            // Setup the serial port to 115200 baud //

 Serial.setDebugOutput(true);                                                                 // Serial.setDebugOutput(true); activates the LOG output to the Serial object, which in this case is attached to the USB CDC port.
 Tekstprintln("Serial started at 9600 baud");                                                 // Turn on Red LED
 SetStatusLED(0,0,0,0,10);
 delay(1000);
                         #ifdef ESP32C3_DEV
 pinMode(REDPIN,     OUTPUT);
 pinMode(GREENPIN,   OUTPUT);
 pinMode(BLUEPIN,    OUTPUT);
 pinMode(COLD_WHITE, OUTPUT);
 pinMode(WARM_WHITE, OUTPUT);     
                        #endif //ESP32C3_DEV    
                        #ifdef ESP32C3_SUPERMINI
 pinMode(LEDPIN,     OUTPUT);
                        #endif //ESP32C3_SUPERMINI

                        #ifdef NANOESP32 
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE,  OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
                        #endif //NANOESP32
                        #ifdef ESP32S3_ZERO
  strip.begin();                                                                              // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();                                                                               // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
  for(uint32_t i=0; i<255; i++){strip.setPixelColor(0, i*250);strip.show();  delay(2); }
                           #endif // ESP32S3_ZERO  

 SetStatusLED(0,0,10,0,0);                                                                    // Set the status LED to red
 InitStorage();                           Tekstprintln("Setting loaded");                     // Load settings from storage and check validity 
 if(Mem.BLEOn) StartBLEService();         Tekstprintln("BLE started");                        // Start BLE service
 if(Mem.WIFIOn){ Tekstprintln("Starting WIFI");  StartWIFI_NTP();   }                         // Start WIFI and optional NTP if Mem.WIFIOn = 1      
 SWversion();                                                                                 // Print the menu + version 
 GetTijd(0);                                                                                  // 
 Print_RTC_tijd();
 PrintTimeToSerial1(); 
 msTick = millis(); 
}
//--------------------------------------------                                                //
// ARDUINO Loop
//--------------------------------------------
void loop() 
{
 Loopcounter++;
 EverySecondCheck();  
 CheckDevices();
}
//--------------------------------------------
// Common Check connected input devices
//--------------------------------------------
void CheckDevices(void)
{
 CheckBLE();                                                                                  // Something with BLE to do?
 SerialCheck();                                                                               // Check serial port every second 
 Serial1Check();                                                                               // Check serial port every second 
 ElegantOTA.loop();                                                                           // For Over The Air updates This loop block is necessary for ElegantOTA to handle reboot after OTA update.
}
//--------------------------------------------                                                //
// CLOCK Update routine done every second
//--------------------------------------------
void EverySecondCheck(void)
{
 static int Toggle = 1;
 uint32_t msLeap = millis() - msTick;                                                         // 
 if (msLeap >999)                                                                             // Every second enter the loop
   {
    msTick = millis();
    GetTijd(0);                                                                               // get the time for the seconds 
    Toggle = 1-Toggle;   
    UpdateStatusLEDs(Toggle);
    if (timeinfo.tm_min != lastminute) EveryMinuteUpdate();                                   // Enter the every minute routine after one minute; 
    Loopcounter=0;
   }    
 }
//--------------------------------------------
// CLOCK Update routine done every minute
//-------------------------------------------- 
void EveryMinuteUpdate(void)
{   
 lastminute = timeinfo.tm_min;  
 if(timeinfo.tm_min % 6 == 3)  PrintTimeToSerial1();                                          // Print the Ttimestring to the serial port
 //  if(WiFi.localIP()[0]>0) WIFIConnected = true; else WIFIConnected = false;                // To be sure connection is still there
 if(timeinfo.tm_hour != lasthour) EveryHourUpdate();
}
//--------------------------------------------
// CLOCK Update routine done every hour
//--------------------------------------------
void EveryHourUpdate(void)
{
 if(WiFi.localIP()[0]==0)
   {
     sprintf(sptext, "Disconnected from station, attempting reconnection");
     Tekstprintln(sptext);
     WiFi.reconnect();
   }
 lasthour = timeinfo.tm_hour;
 if (!Mem.StatusLEDsOn) SetStatusLED(0,0,0,0,0);                                               // If for some reason the LEDs are ON and after a MCU restart turn them off.  
 if (timeinfo.tm_mday != lastday) EveryDayUpdate();  
}
//--------------------------------------------                      SetStatusLED               //
// CLOCK Update routine done every day
//------------------------------------------------------------------------------
void EveryDayUpdate(void)
{
 if(timeinfo.tm_mday != lastday) 
   {
    lastday = timeinfo.tm_mday; 
//    StoreStructInFlashMemory();                                                             // 
    }
}
//--------------------------------------------                                                //
// COMMON Update routine for the status LEDs
//-------------------------------------------- 
void UpdateStatusLEDs(int Toggle)
{
 if (Mem.StatusLEDsOn)   
     {
   if(Toggle) 
         { 
          SetStatusLED(0,0,0,WIFIConnected*10,deviceConnected*10);
          digitalWrite(LED_BUILTIN, LOW); 
         }
    else     
        {
          SetStatusLED(0,0,0,0,0);
         digitalWrite(LED_BUILTIN, HIGH);                                                   // on Nano ESP32 Turn the LED on
        }

     }
     else
     {
      SetStatusLED(0,0,0,0,0); 
      digitalWrite(LED_BUILTIN, HIGH);                                                  // Turn the LED off by making the voltage HIGH
     }
}
//--------------------------------------------
// Common check for serial input
//--------------------------------------------
void SerialCheck(void)
{
 String SerialString; 
 while (Serial.available())
    { 
     char c = Serial.read();                                                                  // Serial.write(c);
     if (c>31 && c<128) SerialString += c;                                                    // Allow input from Space - Del
     else c = 0;                                                                              // Delete a CR
    }
 if (SerialString.length()>0) 
    {
     Serial1.println(SerialString);
     ReworkInputString(SerialString);  
     SerialString = "";
    }
}
//--------------------------------------------
// Common check for serial input
//--------------------------------------------
void Serial1Check(void)
{
 String SerialString; 
 while (Serial1.available())
    { 
     char c = Serial1.read();                                                                  // Serial.write(c);
     SerialString += c;                                                                        // Allow input from Space - Del
     if (c==13) break;                                                                         // exit on CR
    }
 if (SerialString.length()>0) 
    {
     sprintf(sptext,"%s",SerialString.c_str());    Tekstprint(sptext);                        // Print to Ser0/USB and BLE 
     SerialString = "";
    }
}
//--------------------------------------------                                                //
// Common Reset to default settings
//------------------------------------------------------------------------------
void Reset(void)
{
 Mem.Checksum         = 25065;                                                                //
 Mem.TurnOffLEDsAtHH  = 0;                                                                    // Display Off at nn hour
 Mem.TurnOnLEDsAtHH   = 0;                                                                    // Display On at nn hour Not Used
 Mem.BLEOn            = 1;                                                                    // BLE On
 Mem.UseBLELongString = 0;
 Mem.NTPOn            = 0;                                                                    // NTP On
 Mem.WIFIOn           = 0;                                                                    // WIFI On  
 Mem.ReconnectWIFI    = 0;                                                                    // Correct time if necesaary in seconds
 Mem.WIFINoOfRestarts = 0;                                                                    //  
 Mem.UseSDcard        = 0;

 strcpy(Mem.Ssid,"");                                                                         // Default SSID maxlen = 30
 strcpy(Mem.Password,"");                                                                     // Default password maxlen = 40
 strcpy(Mem.BLEbroadcastName,"ESP32WordClock");                                               // MaxLen string is 30
 strcpy(Mem.Timezone,"CET-1CEST,M3.5.0,M10.5.0/3");                                           // Central Europe, Amsterdam, Berlin etc.
 // For debugging
// strcpy(Mem.Ssid,"Guest");
// strcpy(Mem.Password,"guest_001");
// Mem.NTPOn        = 1;                                                                     // NTP On
// Mem.WIFIOn       = 1;                                                                     // WIFI On  

 Tekstprintln("**** Reset of preferences ****"); 
 StoreStructInFlashMemory();                                                                  // Update Mem struct       
 GetTijd(0);                                                                                  // Get the time and store it in the proper variables
 SWversion();                                                                                 // Display the version number of the software
}
//--------------------------------------------                                                //
// Common common print routines
//--------------------------------------------
void Tekstprint(char const *tekst)    { if(Serial) Serial.print(tekst); SendMessageBLE(tekst); sptext[0]=0;   } //
void Tekstprintln(char const *tekst)  { sprintf(sptext,"%s\n",tekst); Tekstprint(sptext);  }
void TekstSprint(char const *tekst)   { printf(tekst); sptext[0]=0;}                          // printing for Debugging purposes in serial monitor 
void TekstSprintln(char const *tekst) { sprintf(sptext,"%s\n",tekst); TekstSprint(sptext); }

//------------------------------------------------------------------------------
//  Common Constrain a string with integers
// The value between the first and last character in a string is returned between the low and up bounderies
//------------------------------------------------------------------------------
int SConstrainInt(String s,byte first,byte last,int low,int up){return constrain(s.substring(first, last).toInt(), low, up);}
int SConstrainInt(String s,byte first,          int low,int up){return constrain(s.substring(first).toInt(), low, up);}
//--------------------------------------------                                                //
// Common Init and check contents of EEPROM
//--------------------------------------------
void InitStorage(void)
{
 // if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){ Tekstprintln("Card Mount Failed");   return;}
 // else Tekstprintln("SPIFFS mounted"); 

 GetStructFromFlashMemory();
 if( Mem.Checksum != 25065)
   {
    sprintf(sptext,"Checksum (25065) invalid: %d\n Resetting to default values",Mem.Checksum); 
    Tekstprintln(sptext); 
    Reset();                                                                                  // If the checksum is NOK the Settings were not set
   }
 if(strlen(Mem.Password)<5 || strlen(Mem.Ssid)<3)     Mem.WIFIOn = Mem.NTPOn = 0;       // If ssid or password invalid turn WIFI/NTP off
 
 StoreStructInFlashMemory();
}
//--------------------------------------------                                                //
// COMMON Store mem.struct in FlashStorage or SD
// Preferences.h  
//--------------------------------------------
void StoreStructInFlashMemory(void)
{
  FLASHSTOR.begin("Mem",false);       //  delay(100);
  FLASHSTOR.putBytes("Mem", &Mem , sizeof(Mem) );
  FLASHSTOR.end();          
  
// Can be used as alternative
//  SPIFFS
//  File myFile = SPIFFS.open("/MemStore.txt", FILE_WRITE);
//  myFile.write((byte *)&Mem, sizeof(Mem));
//  myFile.close();
 }
//--------------------------------------------
// COMMON Get data from FlashStorage
// Preferences.h
//--------------------------------------------
void GetStructFromFlashMemory(void)
{
 FLASHSTOR.begin("Mem", false);
 FLASHSTOR.getBytes("Mem", &Mem, sizeof(Mem) );
 FLASHSTOR.end(); 

// Can be used as alternative if no SD card
//  File myFile = SPIFFS.open("/MemStore.txt");  FILE_WRITE); myFile.read((byte *)&Mem, sizeof(Mem));  myFile.close();

 sprintf(sptext,"Mem.Checksum = %d",Mem.Checksum);Tekstprintln(sptext); 
}
//--------------------------------------------                                                //
//  CLOCK Input from Bluetooth or Serial
//--------------------------------------------
void ReworkInputString(String InputString)
{    
 if(InputString[0] != '?')     {
        return;
    }                                                                                         // String for this menu must start with a '?'  
 InputString = InputString.substring(1);                                                      // remove the '?'
 if(InputString.length()> 40){Serial.printf("Input string too long (max40)\n"); return;}      // If garbage return
 for (int n=0; n<InputString.length()+1; n++)                                                 // remove CR and LF
    if (InputString[n] == 10 || InputString[n]==13) InputString.remove(n,1);
 sptext[0] = 0;                                                                                     // Empty the sptext string
 if(InputString[0] > 31 && InputString[0] <127)                                               // Does the string start with a letter?
  { 
  switch (InputString[0])
   {
    case 'A':
    case 'a': 
            if (InputString.length() >4 )
              {
               InputString.substring(1).toCharArray(Mem.Ssid,InputString.length());
               sprintf(sptext,"SSID set: %s", Mem.Ssid);  
              }
            else sprintf(sptext,"**** Length fault. Use between 4 and 30 characters ****");
            break;
    case 'B':
    case 'b': 
            if (InputString.length() >4 )
              {  
               InputString.substring(1).toCharArray(Mem.Password,InputString.length());
               sprintf(sptext,"Password set: %s\n Enter ?@ to reset ESP32 and connect to WIFI and NTP", Mem.Password); 
               Mem.NTPOn        = 1;                                                          // NTP On
               Mem.WIFIOn       = 1;                                                          // WIFI On  
              }
            else sprintf(sptext,"%s,**** Length fault. Use between 4 and 40 characters ****",Mem.Password);
            break;   
    case 'C':
    case 'c': 
            if (InputString.length() >4 )
              {  
               InputString.substring(1).toCharArray(Mem.BLEbroadcastName,InputString.length());
               sprintf(sptext,"BLE broadcast name set: %s", Mem.BLEbroadcastName); 
               Mem.BLEOn        = 1;                                                          // BLE On
              }
            else sprintf(sptext,"**** Length fault. Use between 4 and 30 characters ****");
            break;      
    case 'D':
    case 'd':  
            if (InputString.length() == 9 )
              {
               timeinfo.tm_mday = (int) SConstrainInt(InputString,1,3,0,31);
               timeinfo.tm_mon  = (int) SConstrainInt(InputString,3,5,0,12) - 1; 
               timeinfo.tm_year = (int) SConstrainInt(InputString,5,9,2000,9999) - 1900;                                              // Date will be printed in Digital display mode
               SetRTCTime();
               Print_tijd();  //Tekstprintln(sptext);                                         // Print_Tijd() fills sptext with time string
              }
            else sprintf(sptext,"****\nLength fault. Enter Dddmmyyyy\n****");
            break;
    case 'F':
    case 'f':  
             break;
    case 'G':
    case 'g':  
             break;
    case 'H':
    case 'h':  
             break;            
    case 'I':
    case 'i': 
            SWversion();
            break;
    case 'K':
    case 'k':
             break;      
    case 'L':                                                                                 
    case 'l':
            break;     
    case 'N':
    case 'n':
             if (InputString.length() == 1 )         Mem.TurnOffLEDsAtHH = Mem.TurnOnLEDsAtHH = 0;
             if (InputString.length() == 5 )
              {
               Mem.TurnOffLEDsAtHH =(byte) InputString.substring(1,3).toInt(); 
               Mem.TurnOnLEDsAtHH = (byte) InputString.substring(3,5).toInt(); 
              }
             Mem.TurnOffLEDsAtHH = _min(Mem.TurnOffLEDsAtHH, 23);
             Mem.TurnOnLEDsAtHH  = _min(Mem.TurnOnLEDsAtHH, 23); 
             sprintf(sptext,"Display is OFF between %2d:00 and %2d:00", Mem.TurnOffLEDsAtHH,Mem.TurnOnLEDsAtHH );
             break;
    case 'O':
    case 'o':
             break;                                                               
    case 'p':
    case 'P':  
             if(InputString.length() == 1)
               {
                Mem.StatusLEDsOn = !Mem.StatusLEDsOn;
                if (!Mem.StatusLEDsOn ) SetStatusLED(0,0,0,0,0); 
                sprintf(sptext,"StatusLEDs are %s", Mem.StatusLEDsOn?"ON":"OFF" );               
               }
             break;    
    case 'q':
    case 'Q':  
            break;
    case 'R':
    case 'r':
             if (InputString.length() == 1)
               {   
                Reset();
                sprintf(sptext,"\nReset to default values: Done");
               }                                
             else sprintf(sptext,"**** Length fault. Enter R ****");
             break;      
    case 'S':                                                                                 // factor ( 0 - 1) to multiply brighness (0 - 255) with 
    case 's':

              break;                    
    case 'T':
    case 't':
//                                                                                            //
             if(InputString.length() == 7)  // T125500
               {
                timeinfo.tm_hour = (int) SConstrainInt(InputString,1,3,0,23);
                timeinfo.tm_min  = (int) SConstrainInt(InputString,3,5,0,59); 
                timeinfo.tm_sec  = (int) SConstrainInt(InputString,5,7,0,59);
                SetRTCTime();
                Print_tijd(); 
               }
             else sprintf(sptext,"**** Length fault. Enter Thhmmss ****");
             break;            
    case 'V':
    case 'v':  
             break;      
    case 'W':
    case 'w':
             if (InputString.length() == 1)
               {   
                Mem.WIFIOn = 1 - Mem.WIFIOn; 
                Mem.ReconnectWIFI = 0;                                                       // Reset WIFI reconnection counter 
                sprintf(sptext,"WIFI is %s after restart", Mem.WIFIOn?"ON":"OFF" );
               }                                
             else sprintf(sptext,"**** Length fault. Enter W ****");
             break; 
    case 'X':
    case 'x':
             if (InputString.length() == 1)
               {   
                Mem.NTPOn = 1 - Mem.NTPOn; 
                sprintf(sptext,"NTP is %s after restart", Mem.NTPOn?"ON":"OFF" );
               }                                
             else sprintf(sptext,"**** Length fault. Enter X ****");
             break; 
    case 'Y':
    case 'y':
             if (InputString.length() == 1)
               {   
                Mem.BLEOn = 1 - Mem.BLEOn; 
                sprintf(sptext,"BLE is %s after restart", Mem.BLEOn?"ON":"OFF" );
               }                                
             else sprintf(sptext,"**** Length fault. Enter Y ****");
             break; 
    case 'Z':
    case 'z':
             if (InputString.length() == 1)
               {   
                Mem.UseBLELongString = 1 - Mem.UseBLELongString; 
                sprintf(sptext,"Fast BLE is %s", Mem.UseBLELongString?"ON":"OFF" );
               }                                
             else sprintf(sptext,"**** Length fault. Enter U ****");
             break; 
        
    case '@':
             if (InputString.length() == 1)
               {   
               Tekstprintln("\n*********\n ESP restarting\n*********\n");
                ESP.restart();   
               }                                
             else sprintf(sptext,"**** Length fault. Enter @ ****");
             break;     
    case '#':
             break; 
    case '$':
             break; 
    case '&':                                                            // Get NTP time
             if (InputString.length() == 1)
              {
               NTP.getTime();                                           // Force a NTP time update      
               Tekstprint("NTP query started. \nClock time: ");
               delay(500);          
               GetTijd(1);
               Tekstprint("\n  NTP time: ");
               PrintNTP_tijd();
               Tekstprintln("");
               } 
             break;
    case '0':
    case '1':
    case '2':        
             if (InputString.length() == 6 )                                                  // For compatibility input with only the time digits
              {
               timeinfo.tm_hour = (int) SConstrainInt(InputString,0,2,0,23);
               timeinfo.tm_min  = (int) SConstrainInt(InputString,2,4,0,59); 
               timeinfo.tm_sec  = (int) SConstrainInt(InputString,4,6,0,59);
               sprintf(sptext,"Time set");  
               SetRTCTime();
               Print_RTC_tijd(); 
               } 
    default: break;
    }
  }  
 Tekstprintln(sptext); 
 StoreStructInFlashMemory();                                                                   // Update EEPROM                                     
 InputString = "";
}

//--------------------------------------------
// CLOCK Control the LEDs on the ESP32
// -1 leaves intensity untouched
//  0 turn LED off
//  1-255 turns LED on with intensity if available
//--------------------------------------------
void SetStatusLED(int WW, int CW, int Re, int Gr, int Bl)
{
//  sprintf(sptext,"WW:%d, CW:%d, Re:%d, Gr:%d, Bl:%d", WW,CW,Re,Gr,Bl); Tekstprintln(sptext); 

                               #ifdef ESP32C3_SUPERMINI
 if(Gr >=0 ) digitalWrite(LEDPIN, Gr);
                               #endif //ESP32C3_SUPERMINI
                               #ifdef ESP32S3_ZERO
 uint32_t color = strip.getPixelColor(0);
 if (WW <0) WW = Cwhite(color);
 if (CW <0) CW = Cwhite(color);
 if (Re <0) Re = Cred(color);
 if (Gr <0) Gr = Cgreen(color);
 if (Bl <0) Bl = Cblue(color);
 strip.setPixelColor(0, strip.Color(Re, Gr, Bl, CW)); 
 strip.show(); 
                                #endif //ESP32S3_ZERO
                                #ifdef ESP32C3_DEV 
 if(Re >=0 ) analogWrite(REDPIN,     Re);
 if(Gr >=0 ) analogWrite(GREENPIN,   Gr);
 if(Bl >=0 ) analogWrite(BLUEPIN,    Bl);
 if(CW >=0 ) analogWrite(COLD_WHITE, CW);
 if(WW >=0 ) analogWrite(WARM_WHITE, WW);
                                #endif
                                #ifdef NANOESP32 
  if(Re >0) digitalWrite(LED_RED,   LOW); 
  if(Re==0) digitalWrite(LED_RED,   HIGH);                         // LOW is On and HIGH is LED off
  if(Gr >0) digitalWrite(LED_GREEN, LOW); 
  if(Gr==0) digitalWrite(LED_GREEN, HIGH);
  if(Bl >0) digitalWrite(LED_BLUE,  LOW); 
  if(Bl==0) digitalWrite(LED_BLUE,  HIGH);
                                #endif //NANOESP32
}

//--------------------------------------------
//  LED Strip Get Pixel Color 
//--------------------------------------------
uint32_t StripGetPixelColor(int Lednr)
{
//return(strip.getPixelColor(Lednr));
return(0);
}
//--------------------------------------------                                                //
//  LED convert HSV to RGB  h is from 0-360, s,v values are 0-1
//  r,g,b values are 0-255
//--------------------------------------------
uint32_t HSVToRGB(double H, double S, double V) 
{
  int i;
  double r, g, b, f, p, q, t;
  if (S == 0)  {r = V;  g = V;  b = V; }
  else
  {
    H >= 360 ? H = 0 : H /= 60;
    i = (int) H;
    f = H - i;
    p = V * (1.0 -  S);
    q = V * (1.0 - (S * f));
    t = V * (1.0 - (S * (1.0 - f)));
    switch (i) 
    {
     case 0:  r = V;  g = t;  b = p;  break;
     case 1:  r = q;  g = V;  b = p;  break;
     case 2:  r = p;  g = V;  b = t;  break;
     case 3:  r = p;  g = q;  b = V;  break;
     case 4:  r = t;  g = p;  b = V;  break;
     default: r = V;  g = p;  b = q;  break;
    }
  }
return FuncCRGBW((int)(r*255), (int)(g*255), (int)(b*255), 0 );                                // R, G, B, W 
}
//--------------------------------------------                                                //
//  LED function to make RGBW color
//--------------------------------------------
uint32_t FuncCRGBW( uint32_t Red, uint32_t Green, uint32_t Blue, uint32_t White)
{ 
 return ( (White<<24) + (Red << 16) + (Green << 8) + Blue );
}
//--------------------------------------------                                                //
//  LED functions to extract RGBW colors
//--------------------------------------------
 uint8_t Cwhite(uint32_t c) { return (c >> 24);}
 uint8_t Cred(  uint32_t c) { return (c >> 16);}
 uint8_t Cgreen(uint32_t c) { return (c >> 8); }
 uint8_t Cblue( uint32_t c) { return (c);      }




//                                                                                            //
//--------------------------------------------
//  COMMON String upper
//--------------------------------------------
void to_upper(char* string)
{
 const char OFFSET = 'a' - 'A';
 while (*string)
  {
   (*string >= 'a' && *string <= 'z') ? *string -= OFFSET : *string;
   string++;
  }
}

//--------------------------------------------                                                //
// CLOCK Version info
//--------------------------------------------
void SWversion(void) 
{ 
 #define FILENAAM (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
 PrintLine(35);
 for (uint8_t i = 0; i < sizeof(menu) / sizeof(menu[0]); Tekstprintln(menu[i++]));
 PrintLine(35);
 sprintf(sptext,"SSID: %s", Mem.Ssid);                                              Tekstprintln(sptext);
// sprintf(sptext,"Password: %s", Mem.Password);                                    Tekstprintln(sptext);
 sprintf(sptext,"BLE name: %s", Mem.BLEbroadcastName);                              Tekstprintln(sptext);
 sprintf(sptext,"IP-address: %d.%d.%d.%d (/update)", 
     WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );  Tekstprintln(sptext);
 sprintf(sptext,"Timezone:%s", Mem.Timezone);                                       Tekstprintln(sptext); 
 sprintf(sptext,"%s %s %s %s", Mem.WIFIOn?"WIFI=On":"WIFI=Off", 
                               Mem.NTPOn?  "NTP=On":"NTP=Off",
                               Mem.BLEOn?  "BLE=On":"BLE=Off",
                               Mem.UseBLELongString? "FastBLE=On":"FastBLE=Off" );  Tekstprintln(sptext);

 sprintf(sptext,"Software: %s",FILENAAM);                                           Tekstprintln(sptext);  //VERSION); 
 Print_RTC_tijd(); Tekstprintln(""); 
 PrintLine(35);
}
void PrintLine(byte Lengte)
{
 for(int n=0; n<Lengte; n++) sptext[n]='_';
 sptext[Lengte] = 0;
 Tekstprintln(sptext);
}

//--------------------------- Time functions --------------------------
//--------------------------------------------                                                //
// COMMON Print T+time string   
// to the serial port
//--------------------------------------------
void PrintTimeToSerial1(void)
{
  if (timeinfo.tm_hour+timeinfo.tm_min == 0)  return;
 sprintf(sptext,"T%02d%02d%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
 if(Serial1) Serial1.println(sptext);
 Tekstprintln(sptext); 
}

//--------------------------------------------                                                //
// RTC Get time from NTP cq RTC 
// and store it in timeinfo struct
//--------------------------------------------
time_t GetTijd(byte printit)
{
 time_t now;
 if(Mem.NTPOn)  getLocalTime(&timeinfo);                                                   // if NTP is running get the local time
 else { time(&now); localtime_r(&now, &timeinfo);}                                            // else get the time from the internal RTC 
 if (printit)  Print_RTC_tijd();                                                              // 
 localtime(&now);
 return now;                                                                                  // return the unixtime in seconds
}
//--------------------------------------------                                                //
// RTC prints time to serial
//--------------------------------------------
void Print_RTC_tijd(void)
{
 sprintf(sptext,"%02d/%02d/%04d %02d:%02d:%02d ", 
     timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,
     timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
 Tekstprint(sptext);
}
//--------------------------------------------
// NTP print the NTP time for the timezone set 
//--------------------------------------------
void PrintNTP_tijd(void)
{
 sprintf(sptext,"%s  ", NTP.getTimeDateString());  
 Tekstprint(sptext);              // 17/10/2022 16:08:15

// int dd,mo,yy,hh,mm,ss=0;      // nice method to extract the values from a string into vaiabeles
// if (sscanf(sptext, "%2d/%2d/%4d %2d:%2d:%2d", &dd, &mo, &yy, &hh, &mm, &ss) == 6) 
//     {      sprintf(sptext,"%02d:%02d:%02d %02d-%02d-%04d", hh,mm,ss,dd,mo,yy);      Tekstprintln(sptext); }
}

//--------------------------------------------
// NTP print the NTP UTC time 
//--------------------------------------------
void PrintUTCtijd(void)
{
 time_t tmi;
 struct tm* UTCtime;
 time(&tmi);
 UTCtime = gmtime(&tmi);
 sprintf(sptext,"UTC: %02d:%02d:%02d %02d-%02d-%04d  ", 
     UTCtime->tm_hour,UTCtime->tm_min,UTCtime->tm_sec,
     UTCtime->tm_mday,UTCtime->tm_mon+1,UTCtime->tm_year+1900);
 Tekstprint(sptext);   
}


//--------------------------------------------
// NTP processSyncEvent 
//--------------------------------------------
void processSyncEvent (NTPEvent_t ntpEvent) 
{
 switch (ntpEvent.event) 
    {
        case timeSyncd:
        case partlySync:
        case syncNotNeeded:
        case accuracyError:
            sprintf(sptext,"[NTP-event] %s\n", NTP.ntpEvent2str (ntpEvent));
            Tekstprint(sptext);
            break;
        default:
            break;
    }
}
//--------------------------------------------                                                //
// RTC Fill sptext with time
//--------------------------------------------
void Print_tijd(){ Print_tijd(2);}                                                            // print with linefeed
void Print_tijd(byte format)
{
 sprintf(sptext,"%02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
 switch (format)
 {
  case 0: break;
  case 1: Tekstprint(sptext); break;
  case 2: Tekstprintln(sptext); break;  
 }
}

//--------------------------------------------
// RTC Set time from global timeinfo struct
// Check if values are within range
//--------------------------------------------
void SetRTCTime(void)
{ 
 time_t t = mktime(&timeinfo);  // t= unixtime
 sprintf(sptext, "Setting time: %s", asctime(&timeinfo));    Tekstprintln(sptext);
 struct timeval now = { .tv_sec = t , .tv_usec = 0};
 settimeofday(&now, NULL);
 GetTijd(0);                                                                                // Synchronize time with RTC clock
 Print_tijd();
}

//--------------------------------------------                                                //
// Print all the times available 
//--------------------------------------------
void PrintAllClockTimes(void)
{
 Tekstprint(" Clock time: ");
 GetTijd(1);
 Tekstprint("\n   NTP time: ");
 PrintNTP_tijd();
 Tekstprintln(""); 
}
//                                                                                            //
// ------------------- End  Time functions 

//--------------------------------------------
//  CLOCK Convert Hex to uint32
//--------------------------------------------
uint32_t HexToDec(String hexString) 
{
 uint32_t decValue = 0;
 int nextInt;
 for (uint8_t i = 0; i < hexString.length(); i++) 
  {
   nextInt = int(hexString.charAt(i));
   if (nextInt >= 48 && nextInt <= 57)  nextInt = map(nextInt, 48, 57, 0, 9);
   if (nextInt >= 65 && nextInt <= 70)  nextInt = map(nextInt, 65, 70, 10, 15);
   if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
   nextInt = constrain(nextInt, 0, 15);
   decValue = (decValue * 16) + nextInt;
  }
 return decValue;
}

//--------------------------------------------                                                //
// BLE 
// SendMessage by BLE Slow in packets of 20 chars
// or fast in one long string.
// Fast can be used in IOS app BLESerial Pro
//------------------------------
void SendMessageBLE(std::string Message)
{
 if(deviceConnected) 
   {
    if (Mem.UseBLELongString)                                                                 // If Fast transmission is possible
     {
      pTxCharacteristic->setValue(Message); 
      pTxCharacteristic->notify();
      delay(10);                                                                              // Bluetooth stack will go into congestion, if too many packets are sent
     } 
   else                                                                                       // Packets of max 20 bytes
     {   
      int parts = (Message.length()/20) + 1;
      for(int n=0;n<parts;n++)
        {   
         pTxCharacteristic->setValue(Message.substr(n*20, 20)); 
         pTxCharacteristic->notify();
         delay(40);                                                                           // Bluetooth stack will go into congestion, if too many packets are sent
        }
     }
   } 
}

//-----------------------------
// BLE Start BLE Classes
//------------------------------
class MyServerCallbacks: public BLEServerCallbacks 
{
 void onConnect(BLEServer* pServer) {deviceConnected = true; };
 void onDisconnect(BLEServer* pServer) {deviceConnected = false;}
};

class MyCallbacks: public BLECharacteristicCallbacks 
{
 void onWrite(BLECharacteristic *pCharacteristic) 
  {
   std::string rxValue = pCharacteristic->getValue();
   ReceivedMessageBLE = rxValue + "\n";
//   if (rxValue.length() > 0) {for (int i = 0; i < rxValue.length(); i++) printf("%c",rxValue[i]); }
//   printf("\n");
  }  
};
//--------------------------------------------                                                //
// BLE Start BLE Service
//------------------------------
void StartBLEService(void)
{
 BLEDevice::init(Mem.BLEbroadcastName);                                                       // Create the BLE Device
 pServer = BLEDevice::createServer();                                                         // Create the BLE Server
 pServer->setCallbacks(new MyServerCallbacks());
 BLEService *pService = pServer->createService(SERVICE_UUID);                                 // Create the BLE Service
 pTxCharacteristic =                                                                          // Create a BLE Characteristic 
     pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);                 
 BLECharacteristic * pRxCharacteristic = 
     pService->createCharacteristic(CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE);
 pRxCharacteristic->setCallbacks(new MyCallbacks());
 pService->start(); 
 BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
 pAdvertising->addServiceUUID(SERVICE_UUID); 
 pServer->start();                                                                            // Start the server  Nodig??
 pServer->getAdvertising()->start();                                                          // Start advertising
 //TekstSprint("BLE Waiting a client connection to notify ...\n"); 
}

//                                                                                            //
//-----------------------------
// BLE  CheckBLE
//------------------------------
void CheckBLE(void)
{
 if(!deviceConnected && oldDeviceConnected)                                                   // Disconnecting
   {
    delay(300);                                                                               // Give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();                                                              // Restart advertising
    TekstSprint("Start advertising\n");
    oldDeviceConnected = deviceConnected;
   }
 if(deviceConnected && !oldDeviceConnected)                                                   // Connecting
   { 
    delay(200);                                                                               // Oterwise text is missing 
    oldDeviceConnected = deviceConnected;
    SWversion();
    Tekstprintln("Enter a ? before commands for this ESP32 menu.");
    Tekstprintln("Otherwise you enter commands for the word clock menu");
    Tekstprintln("'?i' displays this menu and 'i' the word clock menu");
   }
 if(ReceivedMessageBLE.length()>0)
   {
    SendMessageBLE(ReceivedMessageBLE);
    String BLEtext = ReceivedMessageBLE.c_str();
    ReceivedMessageBLE = "";
    Serial1.println(BLEtext);
    ReworkInputString(BLEtext); 
   }
}

//--------------------------------------------                                                //
// WIFI Events Reconnect when Wifi is lost
// An annoance with the standard WIFI connection examples is that the connections 
// often fails after uploading of statrting the MCU
// After an ESP32.reset a connection succeeds
// By using WIFIevents the WIFI manager tries several times to connect and ofter succeeds within two attempt
//--------------------------------------------
void WiFiEvent(WiFiEvent_t event)
{
  sprintf(sptext,"[WiFi-event] event: %d", event); 
  Tekstprintln(sptext);
  WiFiEventInfo_t info;
    switch (event) {
        case ARDUINO_EVENT_WIFI_READY: 
            Tekstprintln("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Tekstprintln("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Tekstprintln("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Tekstprintln("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Tekstprintln("Connected to access point");
            break;
       case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Mem.ReconnectWIFI++;
            sprintf(sptext, "Disconnected from station, attempting reconnection for the %d time", Mem.ReconnectWIFI);
            Tekstprintln(sptext);
 //           sprintf(sptext,"WiFi lost connection. Reason: %d",info.wifi_sta_disconnected.reason); 
 //           Tekstprintln(sptext);
            WiFi.reconnect();
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Tekstprintln("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
             sprintf(sptext, "Obtained IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
            Tekstprintln(sptext);
            WiFiGotIP(event,info);
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Tekstprintln("Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            Tekstprintln("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            Tekstprintln("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            Tekstprintln("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            Tekstprintln("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Tekstprintln("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Tekstprintln("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Tekstprintln("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            sprintf(sptext,"Client disconnected.");                                            // Reason: %d",info.wifi_ap_stadisconnected.reason); 
            Tekstprintln(sptext);

//          Serial.print("WiFi lost connection. Reason: ");
//          Tekstprintln(info.wifi_sta_disconnected.reason);
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Tekstprintln("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Tekstprintln("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Tekstprintln("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Tekstprintln("STA IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP6:
            Tekstprintln("Ethernet IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_START:
            Tekstprintln("Ethernet started");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Tekstprintln("Ethernet stopped");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Tekstprintln("Ethernet connected");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Tekstprintln("Ethernet disconnected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Tekstprintln("Obtained IP address");
            WiFiGotIP(event,info);
            break;
        default: break;
    }}
//--------------------------------------------                                                //
// WIFI GOT IP address 
//--------------------------------------------
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  WIFIConnected = 1;
 if(Mem.WIFIOn) WebPage();                                                                 // Show the web page if WIFI is on
 if(Mem.NTPOn)
   {
    NTP.setTimeZone(Mem.Timezone);                                                            // TZ_Europe_Amsterdam); //\TZ_Etc_GMTp1); // TZ_Etc_UTC 
    NTP.begin();                                                                              // https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv
    Tekstprintln("NTP is On"); 
   } 
}

//--------------------------------------------                                                //
// WIFI WEBPAGE 
//--------------------------------------------
void StartWIFI_NTP(void)
{
// WiFi.disconnect(true);
// WiFi.onEvent(WiFiEvent);   // Using WiFi.onEvent interrupts and crashes screen display while writing the screen
// Examples of different ways to register wifi events
                         //  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
                         //  WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
                         //    {Serial.print("WiFi lost connection. Reason: ");  Serial.println(info.wifi_sta_disconnected.reason); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

 
 WiFi.mode(WIFI_STA);
 WiFi.begin(Mem.Ssid, Mem.Password);
 if (WiFi.waitForConnectResult() != WL_CONNECTED) 
      { 
       if(Mem.WIFINoOfRestarts == 0)                                                           // Try once to restart the ESP and make a new WIFI connection
       {
          Mem.WIFINoOfRestarts = 1;
          StoreStructInFlashMemory();                                                          // Update Mem struct   
          ESP.restart(); 
       }
       else
       {
         Mem.WIFINoOfRestarts = 0;
         StoreStructInFlashMemory();                                                           // Update Mem struct   
         Tekstprintln("WiFi Failed! Enter @ to retry"); 
         WIFIConnected = 0;       
         return;
       }
      }
 else 
      {
       Tekstprint("Web page started\n");
       sprintf(sptext, "IP Address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
       Tekstprintln(sptext); 
       WIFIConnected = 1;
       Mem.WIFINoOfRestarts = 0;
       StoreStructInFlashMemory();                                                            // Update Mem struct   
       // AsyncElegantOTA.begin(&server);                                                     // Start ElegantOTA  old version
       ElegantOTA.begin(&server);                                                             // Start ElegantOTA  new version in 2023  
       if(Mem.NTPOn)
          {
           NTP.setTimeZone(Mem.Timezone);                                                     // TZ_Europe_Amsterdam); //\TZ_Etc_GMTp1); // TZ_Etc_UTC 
           NTP.begin();                                                                       // https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv
           Tekstprintln("NTP is On"); 
         }
      }
 if(Mem.WIFIOn) WebPage();                                                                 // Show the web page if WIFI is on
}
//--------------------------------------------                                                //
// WIFI WEBPAGE 
//--------------------------------------------
void WebPage(void) 
{
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)                                  // Send web page with input fields to client
          { request->send_P(200, "text/html", index_html);  }    );
 server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request)                              // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
       { 
        String inputMessage;    String inputParam;
        if (request->hasParam(PARAM_INPUT_1))                                                 // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
           {
            inputMessage = request->getParam(PARAM_INPUT_1)->value();
            inputParam = PARAM_INPUT_1;
           }
        else 
           {
            inputMessage = "";    //inputMessage = "No message sent";
            inputParam = "none";
           } 
           if (inputMessage.length()>0) 
              {
                Serial1.println(inputMessage);
                ReworkInputString(inputMessage);  
              }

//        sprintf(sptext,"%s",inputMessage);    Tekstprintln(sptext);
//        ReworkInputString(inputMessage+"\n");

  //      inputMessage.toCharArray(sptext, inputMessage.length());  
   //     sprintf(sptext,"%s",inputMessage);    
  //      Tekstprintln(sptext);
 //  **** testen     ReworkInputString(sptext+"\n"); // error: invalid operands of types 'char [140]' and 'const char [2]' to binary 'operator+'
  //      ReworkInputString(sptext);        // Is the \n needed from line above??
        
        request->send_P(200, "text/html", index_html);
       }   );
 server.onNotFound(notFound);
 server.begin();
}

//--------------------------------------------
// WIFI WEBPAGE 
//--------------------------------------------
void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}
//                                                                                            //
