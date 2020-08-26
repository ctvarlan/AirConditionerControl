/*
The use of websocket is inspired from here:
http://www.martyncurrey.com/esp8266-and-the-arduino-ide-part-9-websockets/

 * Intended to be run on an ESP8266
 * !!! AC stands for Air Conditioner and not for Alternating Current !!!
 
 [ToDo]
 [_] To add OTA
 [X] To test the router settings for access from the Internet
 [_] To change the CSS attributes to have the color changed for the button
 [_] To change the CSS attributes to have the color changed for the main
 [_] To add the status of the AC (ON or OFF) in Flash memory to be kept over a reset of nodeMCU
 
 */
 
String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
 
String html_1 = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
  <meta charset='utf-8'>
  <style>
    body     { font-size:160%;} 
    #main    { background-color:Orange; display: table; width: 300px; margin: auto;  padding: 10px 10px 10px 10px; border: 5px solid blue; border-radius: 10px; text-align:center;} 
    #BTN_AC  { width:240px; height:80px; font-size: 150%;}
    p        { font-size: 90%;}
  </style>
 
  <title>Websockets</title>
</head>
<body>
  <div id='main'>
    <h3>Air Conditioner Control</h3>
    <div id='content'>
      <p id='AC_led'>AC is OFF</p>
      <button id='BTN_AC'class="button">ON</button>
    </div>
    <p>Recieved data = <span id='rd'>---</span> </p>
    <br />
   </div>
</body>
 
<script>
  var Socket;
  function init() 
  {
    Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    Socket.onmessage = function(event) { processReceivedCommand(event); };
  }
  
function processReceivedCommand(evt) 
{
    document.getElementById('rd').innerHTML = evt.data;
    if (evt.data ==='0') 
    {  
        document.getElementById('BTN_AC').innerHTML = 'ON';  
        document.getElementById('AC_led').innerHTML = 'AC = OFF (evt)';  
    }
    if (evt.data ==='1') 
    {  
        document.getElementById('BTN_AC').innerHTML = 'OFF'; 
        document.getElementById('AC_led').innerHTML = 'AC = ON (evt)';   
    }
}

  document.getElementById('BTN_AC').addEventListener('click', buttonClicked);
  function buttonClicked()
  {   
    var btn = document.getElementById('BTN_AC')
    var btnText = btn.textContent || btn.innerText;
    if (btnText ==='ON') { btn.innerHTML = 'OFF'; document.getElementById('AC_led').innerHTML = 'AC = ON (click)';  sendText('1'); }  
    else                 { btn.innerHTML = 'ON';  document.getElementById('AC_led').innerHTML = 'AC = OFF (click)'; sendText('0'); }
  }
 
  function sendText(data)
  {
    Socket.send(data);
  } 
 
  window.onload = function(e)
  { 
    init();
  }
</script>
</html>)=====";        //"
 
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Stepper.h>

#define STEPS       1400    //stepsPerRevolution
#define STBY1       D7      //gpio     STBY for motor1
#define STBY2       D8      //gpio     STBY for motor2
#define ON_cmd      D5      //ON    (LEFT   front)
#define OFF_cmd     D6      //OFF   (RIGHT  back)
#define AC_led      D0      //AC on-off LED


byte    on,off,ref1,ref2;       //ref 1 and ref 2 are not yet used as intended

// initialize the stepper library on pins 
Stepper stepper(STEPS, D1,D2,D3,D4);
// <- 

WiFiServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
 
boolean ACstatus = LOW;             //AC is OFF
 
char ssid[] = "yourSSID";             //to be moved to settings.h
char pass[] = "yourPASSWORD";          //to be moved to settings.h
 
void setup()
{
  pinMode(OFF_cmd, INPUT);      //D6 switch from the pin to gnd
  pinMode(ON_cmd, INPUT);       //D5 switch from the pin to gnd
  pinMode(STBY1, OUTPUT);       //D7
  pinMode(STBY2, OUTPUT);       //D8 -> WARNING on the use of D8 (gpio15): it should be seen as HIGH at the boot time, if not then nodeMCU enter in Flashing mode
  pinMode(AC_led, OUTPUT);      //D0 (gpio16) 

  delay(10);

  //set the speed:
  stepper.setSpeed(100);

  digitalWrite(AC_led, ACstatus);   //AC is OFF
 
  Serial.begin(115200);
  Serial.println("\nSerial started at 115200\n");
 
  // Connect to a WiFi network
  Serial.print(F("Connecting to local ssid"));
  WiFi.begin(ssid,pass);
  
  // connection with timeout
  int count = 0; 
  while ( (WiFi.status() != WL_CONNECTED) && count < 17) 
  {
      Serial.print(".");  delay(500);  count++;
  }
 
  if (WiFi.status() != WL_CONNECTED)
  { 
     Serial.print("\nFailed to connect to ");  Serial.println(ssid);
     //ToDo: reboot and try again
     while(1);
  }
 
  Serial.println();
  Serial.print(F("CONNECTED to "));   
  Serial.println(WiFi.localIP()); 
 
  // start a server
  server.begin();
  Serial.println("Server started");
 
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  //set the position to Ref for both steppers
  digitalWrite(STBY1,HIGH);
  Serial.println("Reference 1");
  stepper.step(STEPS);
  ref1 = 1;                     //flag for position "REF" for stepper1
  Serial.println("Done Ref 1"); //to check how long takes this move
  digitalWrite(STBY1,LOW);
  delay(50);
  
  digitalWrite(STBY2,HIGH);
  Serial.println("Reference 2");
  stepper.step(STEPS);
  ref2 = 1;                     //flag for position "REF" for stepper2
  Serial.println("Done Ref 2"); //to check how long takes this move
  digitalWrite(STBY2,LOW);
  delay(50);
}   //setup()
 
 
void loop()
{
    checkSwitch();
 
    webSocket.loop();
 
    WiFiClient client = server.available(); // Check if a client has connected
    if (!client)  {  return;  }
 
    client.flush();
    client.print( header );
    client.print( html_1 ); 
    Serial.println("New page served");
 
    delay(5);
}
  
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length)
{
  if(type == WStype_TEXT)
  {
      if (payload[0] == '0')
      {
          digitalWrite(AC_led, HIGH);   //LED off
          //sent the command to OFF stepper
          AC_off();        
          ACstatus = LOW;               //AC is OFF
          Serial.println("AC is OFF (from web)");
      }
      else if (payload[0] == '1')
      {
          digitalWrite(AC_led, LOW);    //LED on
          //sent the command to ON stepper     
          AC_on();   
          ACstatus = HIGH;              //AC is ON
          Serial.println("AC is ON (from web)");        
      }
  }
 
  else 
  {
    Serial.print("WStype = ");   
    Serial.println(type);  
    Serial.print("WS payload = ");
    for(int i = 0; i < length; i++) { Serial.print((char) payload[i]); }
    Serial.println();
 
  }
}
 
void checkSwitch()
{
    on  = digitalRead(ON_cmd);
        //on = from something else
    off = digitalRead(OFF_cmd);
        //off = from something else

    if((on == 0) && (off == 1))     //cmd is ON
    {
        AC_on();
    }
    
    if((on == 1) && (off == 0))     //cmd is OFF
    {
        AC_off();
    }
    delay(50);  //to be removed, maybe
}

void AC_on()
{
    digitalWrite(STBY1,HIGH);
    Serial.println("Command: ON");
    stepper.step(-STEPS);
    ref1 = 0;                   //out of the ref1 position
    delay(500);
    stepper.step(STEPS);
    ref1 = 1;                   //position is at ref1
    digitalWrite(STBY1,LOW);
    digitalWrite(AC_led, LOW);  //AC_led = LOW -> ON
    webSocket.broadcastTXT("1"); 
    Serial.println("AC is ON");
}
    
void AC_off()
{
    digitalWrite(STBY2, HIGH);
    Serial.println("Command: OFF");
    stepper.step(-STEPS);
    ref2 = 0;
    delay(500);
    stepper.step(STEPS);
    ref2 = 1;
    digitalWrite(STBY2, LOW);
    digitalWrite(AC_led, HIGH); //AC_led = HIGH -> OFF
    webSocket.broadcastTXT("0"); 
    Serial.println("AC is OFF");
}
