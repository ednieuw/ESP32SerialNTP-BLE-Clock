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
A Normal display<br>
B HET IS WAS always off<br>
C HET IS WAS off after 10 secs<br>
D D15122017 is 15 Dec 2017<br>
F DCF or LDR counts/h<br>
G Display DCF-signal<br>
H Use DCF-receiver<br>
I For this info<br>
L Min light intensity (0-255)<br>
M Max light intensity (1-250)<br>
N LEDs OFF between Nhhhh<br>
O LEDs ON/OFF<br>
R Reset to default settings<br>
S Self test<br>
W Test LDR every second<br>
X Demo mode (Xmsec)<br>
</span>
   <form action="/get">
 <strong><input type="submit" value="Send" class="auto-style8" style="height: 22px"></strong>
     &nbsp;<input type="text" name="input1" style="width: 272px"></form><br>
<br">
</body></html>
)rawliteral";
