const char* PARAM_INPUT_1 = "input1";
const char index_html[]  = 
R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP32-Word clock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style type="text/css">
  .auto-style1 {font-family: Verdana, Geneva, Tahoma, sans-serif; background-color: #FFFFFF; color: #000000; }
  .auto-style2 {text-align: center;  }
  .auto-style3 {font-family: Verdana, Geneva, Tahoma, sans-serif; background-color: #FFFFFF; color: #FF0000; }
  .auto-style8 {font-weight: bold;                                background-color: #FFFF00; color: #000000; }    
  </style>
  </head>
  <body>
<span class="auto-style3">
<strong>ESP32-Word Clock</strong><br> 
</span> 
<span class="auto-style1">
 "?A SSID B Password C BLE beacon name",
 "?D Date (D15012021) T Time (T132145)",
 "?E Timezone  (E<-02>2 or E<+01>-1)",
 "?I To print this Info menu",
 "?P Status LED toggle On/Off", 
 "?R Reset settings @ = Reset MCU",
 "?W=WIFI  ?X=NTP& ?Y=BLE  ?Z=Fast BLE", 
</span>
   <form action="/get">
 <strong><input type="submit" value="Send" class="auto-style8" style="height: 22px"></strong>
     &nbsp;<input type="text" name="input1" style="width: 272px"></form><br>
<br">
</body></html>
)rawliteral";
