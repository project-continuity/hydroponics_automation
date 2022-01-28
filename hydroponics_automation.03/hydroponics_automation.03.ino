//Project Continuity - Hydroponics Automation 
//System target is the Espressif ESP32 microcontroller and SSD1306 Display
//I have no idea if I'm allowed to include these libraries but I think they are open source.
//https://project-continuity.com


// Install https://github.com/espressif/arduino-esp32
// Install https://github.com/me-no-dev/AsyncTCP
// Install https://github.com/me-no-dev/ESPAsyncWebServer
// Install https://github.com/ThingPulse/esp8266-oled-ssd1306

#include "SSD1306Wire.h"
#include "ESPAsyncWebServer.h"
#include "WiFiUdp.h"
#include "EEPROM.h"
#include "Wire.h"
#include "WiFi.h"


// Board Setup
#define RELAY_NO    true
#define NUM_RELAYS  5
#define EEPROM_SIZE 512
#define DEBUG_PRINT(a) (Serial.print(a))
#define DEBUG_PRINTLN(a) (Serial.println(a))

int relayGPIOs[NUM_RELAYS] = {12, 14, 25, 26, 27};
const char* ssid = "<your ssid>";
const char* password = "<your password>";
const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

// Display Setup
SSD1306Wire display(0x3c, SDA, SCL);
#define DISPLAY_DURATION 3000

//Globals
int storedTime [NUM_RELAYS][2];
int onTime;
int offTime;
int onOff;
int relay;
String value;

//Web Server
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Automation Station</h2>
  %BUTTONPLACEHOLDER%
  <br><h4>Save To EEPROM</h4><input type="submit" id="storeEEPROM" value="SAVE" onclick="storeEEPROM()" />
  <br><h4>Clear EEPROM</h4><input type="submit" id="clearEEPROM" value="CLEAR" onclick="clearEEPROM()" />
<script defer>
  var t = new Date();
  var o = [4];
  for(i = 0; i < 5; i++){
    o[i] = [1]; }
  
  for(i = 0; i < 5; i++) {
    for(j = 0; j < 2; j++) {
      retrieveValues(i, j);
      console.log("Init Loop Values: ", o[i][j]); } }
  
  setInterval(function(){
    t = new Date();  
    for(i = 0; i < 5; i++) {
      for(j = 0; j < 2; j++) {
        retrieveValues(i, j);
        trigger(i); } } }, 60000);
  
  function retrieveValues(i, j) {
    console.log("Relay is: " + i + " and onOff is: " + j);
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
      if(xhr.readyState === 4) {
        o[i][j] = Number(xhr.responseText);
        console.log("Retrieve Values Response: ", o[i][j]); } }
    xhr.open("GET", "/getAlarm?relay="+i+"&onoff="+j);
    xhr.send(); 
    console.log("Response value is: " + o[i][j]); }

  function toggleCheckbox(i) {
    var xhr = new XMLHttpRequest();
    if(i.checked){ xhr.open("GET", "/update?relay="+i.id+"&state=1", true); }
    else { xhr.open("GET", "/update?relay="+i.id+"&state=0", true); }
    xhr.send(); }

  function trigger(i){
    j = i + 1;
    let currTime = (t.getHours() * 60) + t.getMinutes();
    console.log("Trigger: i and j: ", o[i][0], o[i][1], currTime);
    if(o[i][0] > o[i][1]) {
      console.log("On Time is greater than Off Time");
      if(currTime > o[i][1] && currTime < o[i][0]) {
        console.log("Current time is between offtime and on time, turn off:On>Off "); 
        if(!document.getElementById(j).checked) {
          console.log("Box is already unchecked"); }
        else {
          console.log("Box is checked, unchecking");
          document.getElementById(j).checked = false;
          toggleCheckbox(document.getElementById(j)); } }
      else {
        console.log("Current time is not between offtime and on time, turn on");
        if(document.getElementById(j).checked) {
          console.log("Box is already checked"); }
        else {
          console.log("Box is unchecked, checking");
          document.getElementById(j).checked = true;
          toggleCheckbox(document.getElementById(j)); } } }   
    else{
      console.log("On time is less than off time");
      if(currTime > o[i][0] && currTime < o[i][1]) {
        console.log("Current time is between on time and off time, turn on:On<Off");
        if(document.getElementById(j).checked) {
           console.log("Box is already checked"); }
        else {
          console.log("Box is unchecked, checking");
          document.getElementById(j).checked = true;
          toggleCheckbox(document.getElementById(j)); } }
      else {
        console.log("Current time is not between on time and off time, turn off");
        if(!document.getElementById(j).checked) {
          console.log("Box is already unchecked"); }
        else {
          console.log("Box is checked, unchecking");
          document.getElementById(j).checked = false;
          toggleCheckbox(document.getElementById(j)); } } } } 
  
  function storeEEPROM() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/storeEEPROM?", true);
    xhr.send();
    console.log("Save EEPROM"); }

  function clearEEPROM() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/clearEEPROM?", true);
    xhr.send();
    console.log("Clear EEPROM"); }

  function alarmUpdate(n, j) {
    // i is the relay number
    var i = n - 1;
    if(j == 0) {
      o[i][0] = (Number(document.getElementById('hourOn_' + n).value)* 60) + (Number(document.getElementById('minuteOn_' + n).value));
      document.getElementById('onTime_' + n).innerHTML = timeConvert(o[i][0]);
      console.log("Alarm Update Loaded On Time Value: ", o[i][0]); }
    else if(j == 1) {
      o[i][1] = (Number(document.getElementById('hourOff_' + n).value)* 60) + (Number(document.getElementById('minuteOff_' + n).value));
      document.getElementById('offTime_' + n).innerHTML = timeConvert(o[i][1]);
      console.log("Alarm Update Loaded Off Time Value: ", o[i][1]); }
    else {
      console.log("State Not Applied Correctly, cannot Update"); }
    var xhr = new XMLHttpRequest();
    if(j == 0) {
      xhr.open("GET", "/alarmUpdate?relay="+i+"&onoff="+j+"&ontime="+o[i][0], true); }
    if(j == 1) {
      xhr.open("GET", "/alarmUpdate?relay="+i+"&onoff="+j+"&offtime="+o[i][1], true); }
    xhr.send(); }

  function timeConvert(i) {
    var _hours = Math.floor(i/60).toLocaleString('en-US', {minimumIntegerDigits: 2, useGrouping:false});
    var _minutes = (i%60).toLocaleString('en-US', {minimumIntegerDigits: 2, useGrouping:false});
    var timeLabel = _hours + ":" + _minutes;
    return timeLabel; }
</script>
</body>
</html>
)rawliteral";

void EEWriteInt(int addr, unsigned int value) {
  for(int i = 0; i < 4; i++){
    EEPROM.write(addr + i, 0);
    EEPROM.commit(); }
  EEPROM.writeInt(addr, value);
  DEBUG_PRINTLN(EEPROM.readInt(0)); }

String formattedTime(int stored) {
  String formatted = String((stored / 60)) + ":" + String(stored%60);
  DEBUG_PRINT("Formatted Time String is: ");
  DEBUG_PRINTLN(formatted);
  return formatted; }
  
String relayState(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay-1])){
      return ""; }
    else {
      return "checked"; } }
  else {
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "checked"; }
    else {
      return ""; } }
  return ""; }

String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    for(int i=1; i<=NUM_RELAYS; i++){
      String relayStateValue = relayState(i);
      
      buttons+= "<h4>Relay #" + String(i) + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayStateValue +"><span class=\"slider\"></span></label>" + 
                "<h4>On Time: </h4><h4 id=\"onTime_" + String(i) + "\"> " + formattedTime(storedTime[i-1][0]) + "</h4><input type=\"number\" id=\"hourOn_" + String(i) + "\" min=\"0\" max=\"23\"><input type =\"number\" id=\"minuteOn_" + String(i) + "\" min=\"0\" max=\"59\"><input type=\"submit\" id=\"onTimeSubmit_" + String(i) + "\" value=\"save\" onclick=\"alarmUpdate(" + i + ",0)\">" +
                "<h4>Off Time: </h4><h4 id=\"offTime_" + String(i) + "\"> " + formattedTime(storedTime[i-1][1]) + "</h4><input type=\"number\" id=\"hourOff_" + String(i) + "\" min=\"0\" max=\"23\"><input type =\"number\" id=\"minuteOff_" + String(i) + "\" min=\"0\" max=\"59\"><input type=\"submit\" id=\"offTimeSubmit_" + String(i) + "\" value=\"save\" onclick=\"alarmUpdate(" + i + ",1)\">"; }
    return buttons; }
  return String(); }

void clearEEPROM() {
  int _count = 0;
  while(_count < EEPROM_SIZE){
    EEPROM.writeInt(_count, 0);
    EEPROM.commit();
    _count = _count + 4;
    DEBUG_PRINT(" *** ");
    DEBUG_PRINT(_count); } }

void storeEEPROM() {
  int _count = 0;
  for(int i = 0; i < NUM_RELAYS; i++){
    for(int j = 0; j < 2; j++) {
      DEBUG_PRINT("Current Value: ");
      DEBUG_PRINT(storedTime[i][j]);
      DEBUG_PRINT(" ");
      EEPROM.writeInt(_count, storedTime[i][j]);
      EEPROM.commit();
      _count = _count + 4;
      DEBUG_PRINT(" *Saved[");
      DEBUG_PRINT(i);
      DEBUG_PRINT("][");
      DEBUG_PRINT(j);
      DEBUG_PRINT("]: Value = ");
      DEBUG_PRINT(EEPROM.readInt(_count)); } }
  DEBUG_PRINTLN(); }

void drawDisplay(String IPAddr){ 
  display.init();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 26, IPAddr);
  display.display(); }
  
void setup(){
  Serial.begin(115200);
  DEBUG_PRINTLN("Display Init....");
  EEPROM.begin(EEPROM_SIZE);
  int _count = 0;
  int _value = 0;
  for(int i = 0; i < NUM_RELAYS; i++) {  
    for(int j = 0; j < 2; j++) {
      _value = EEPROM.readInt(_count);
      if(_value >= 0) {
        storedTime[i][j] = _value; }
      else {
        EEPROM.writeInt(_count, 0);
        _value = EEPROM.readInt(_count);        
        storedTime[i][j] = _value; }
      _count = _count + 4; } }
    for(int i=0; i < NUM_RELAYS; i++){
    pinMode(relayGPIOs[i], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i], HIGH); }
    else{
      digitalWrite(relayGPIOs[i], LOW); } }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    DEBUG_PRINTLN("Connecting to WiFi.."); }
  DEBUG_PRINTLN(WiFi.localIP());
  drawDisplay(WiFi.localIP().toString().c_str());
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/storeEEPROM", HTTP_GET, [](AsyncWebServerRequest *request){
    storeEEPROM();
    request->send(200, "text/plain", "OK"); });

  server.on("/clearEEPROM", HTTP_GET, [](AsyncWebServerRequest *request){
     clearEEPROM();
     request->send(200, "text/plain", "OK"); });
     
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        DEBUG_PRINT("NO ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt()); }
      else{
        DEBUG_PRINT("NC ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt()); } }
    else {
      inputMessage = "No message sent";
      inputParam = "none"; }
    DEBUG_PRINTLN(inputMessage + inputMessage2);
    request->send(200, "text/plain", "OK"); });

  server.on("/alarmUpdate", HTTP_GET, [](AsyncWebServerRequest *request){ 
    if(request->hasParam("ontime")) {
      onTime = request->getParam("ontime")->value().toInt();
      DEBUG_PRINTLN(onTime); }
    if(request->hasParam("offtime")) {
      offTime = request->getParam("offtime")->value().toInt(); 
      DEBUG_PRINTLN(offTime); }
    if(request->hasParam("relay")) {
      relay = request->getParam("relay")->value().toInt();
      DEBUG_PRINTLN(relay); }
    if(request->hasParam("onoff")) {
      onOff = request->getParam("onoff")->value().toInt();
      DEBUG_PRINTLN(onOff); }

    if(onOff) {
      storedTime[relay][onOff] = offTime; }
    else if(onOff == 0) {
      storedTime[relay][onOff] = onTime; }
    else {
      DEBUG_PRINTLN("Not Updated, On Off Issue"); }
    DEBUG_PRINTLN(storedTime[relay][onOff]);
    request->send(200, "text/plain", "OK"); 
  });

  server.on("/getAlarm", HTTP_GET, [](AsyncWebServerRequest *request){ 
    String storedValue;
    if(request->hasParam("relay")) {
      relay = request->getParam("relay")->value().toInt();
      DEBUG_PRINT("Relay Number: ");
      DEBUG_PRINTLN(relay); }
    else {
      DEBUG_PRINTLN("Relay Number Not found"); }
    if(request->hasParam("onoff")) {
      onOff = request->getParam("onoff")->value().toInt();
      DEBUG_PRINT("On Off Status: ");
      DEBUG_PRINTLN(onOff); }
    storedValue = storedTime[relay][onOff];
    DEBUG_PRINTLN(storedValue);
    request->send(200, "text/plain", storedValue);
    });
  server.begin(); }

void loop() {  
}
