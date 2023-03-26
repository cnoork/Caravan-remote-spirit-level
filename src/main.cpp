// hoeken meten en printen on serial monitor en web page

#include <Arduino.h>

#include "Wire.h"
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

// current angleX & angleY, updated in loop()
float roll = 0.0;
float rollWeb = 0.0;
float rollPct = 0.0;
float leftHight = 0.0;
float rightHight = 0.0;
String leftColor = "red";
String rightColor = "red";

float pitch = 0.0;
float pitchWeb = 0.0;
float pitchPct = 0.0;
float frontHight = 0.0;
float backHight = 0.0;
String frontColor = "red";
String backColor = "red";

int dotSensetive = 8;
float tolerance = 0.25;
int floatingAvarageCnt = 5;

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates MPU6050 readings every 10 seconds
const long interval = 1000;  

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "noorkw";
const char* password = "Noordeindestraat17";

/* Soft AP network parameters 
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0); */

// DNS server
DNSServer dnsServer;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
    response->print("<p>This is out captive portal front page.</p>");
    response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
    response->printf("<p>Try opening <a href='http://%s/waterpas.html'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
    response->print("</body></html>");
    request->send(response);
  }
};

//<meta http-equiv='refresh' content='1' url='waterpas.html'>
const char waterpas_html[] PROGMEM = R"rawliteral(
<meta http-equiv='refresh' content='1' url='waterpas.html'>
<html style="width:100&#37;;heigth:100&#37;;">
  <body style="width:100&#37;;heigth:100&#37;;margin:0">

    <svg width="100&#37;" height="100&#37;" viewBox="0 0 100 100" preserveAspectRatio="none" style="background-color: whitesmoke">
      <rect x="10" y="25&#37;" rx="3&#37;" ry="3&#37;" width="80&#37;" height="70&#37;" style="fill:red;opacity:0.5" />

      <polygon points="50,5 35,25 65,25" style="fill:gray" />
      <polygon points="50,10 39,25 61,25" style="fill:white" />
      <circle cx="50&#37;" cy="5&#37;" r="3&#37;" fill="gray" />
      <circle cx="50&#37;" cy="5&#37;" r="1&#37;" fill="white" />

      <polygon points="50,60 40,50 60,50" style="fill:%frontColor%" />
      <polygon points="50,60 40,70 60,70" style="fill:%backColor%" />
      <polygon points="50,60 40,50 40,70" style="fill:%leftColor%" />
      <polygon points="50,60 60,50 60,70" style="fill:%rightColor%" />

      <circle cx="50&#37;" cy="60&#37;" r="4&#37;"; stroke="green" stroke-width="0" fill="mediumspringgreen" />
      <circle cx="%rollPct%&#37;" cy="%pitchPct%&#37;" r="3&#37;" stroke="green" stroke-width="0" fill="red" />

      <text text-anchor="start" x="12&#37" y="60&#37">%leftHight%</text>
      <text text-anchor="end" x="87&#37" y="60&#37">%rightHight%</text>
      <text text-anchor="middle" x="50&#37" y="30&#37">%frontHight%</text>
      <text text-anchor="middle" x="50&#37" y="92&#37">%backHight%</text>

      <style>
        <![CDATA[text {font: 4px Arial;}]]>
      </style>
    </svg>
  </body>
</html>)rawliteral";

// Replaces placeholder with MPU6050 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "roll"){
    return String(roll);
  } else if(var == "pitch"){
    return String(pitch);
  } else if(var == "rollPct"){
    return String(rollPct);
  } else if(var == "pitchPct"){
    return String(pitchPct);
  } else if(var == "frontHight" && frontHight != 0){
    return String(frontHight, 1) + "cm";
  } else if(var == "backHight" && backHight != 0){
    return String(backHight, 1) + "cm";
  } else if(var == "leftHight" && leftHight != 0){
    return String(leftHight, 1) + "cm";
  } else if(var == "rightHight" && rightHight != 0){
    return String(rightHight, 1) + "cm";
  } else if(var == "frontColor"){
    return String(frontColor);
  } else if(var == "backColor"){
    return String(backColor);
  } else if(var == "leftColor"){
    return String(leftColor);
  } else if(var == "rightColor"){
    return String(rightColor);
  }
  return String();
}

void setup() {
  Serial.begin(115200);

  Wire.begin();
  byte status = mpu.begin(0, 0);
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while(status!=0){ } // stop everything if could not connect to MPU6050
  
  // Connect to Wi-Fi
/*  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n");

  // Print ESP8266 Local IP Address
  Serial.print("LAN IP address: ");
  Serial.println(WiFi.localIP()); */

  Serial.println("Setting soft access point mode");
  WiFi.softAP("waterpas");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  delay(5000);
  Serial.print("\n");

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());


  // Route for root / web page
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.on("/waterpas.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", waterpas_html, processor);
  });

  // Start server
  server.begin();
}

void loop() {
  //DNS
//  dnsServer.processNextRequest();

  mpu.update();

  if(millis() - previousMillis > interval){ // print data every second
    previousMillis = millis();

    float pitch = mpu.getAngleX();
    float roll = mpu.getAngleY();

    Serial.println(F("===================== v2f ================================"));

    if (isnan(roll)) {
      Serial.println("Failed to read from MPU6050 sensor!");
    }
    else {
      if(fabsf(fabsf(roll) - fabsf(rollWeb)) < 0.15) {
        rollWeb = ((floatingAvarageCnt -1) * rollWeb + roll) / floatingAvarageCnt;
      } else {
        rollWeb = roll;
      }
      if(rollWeb > tolerance) {
        leftColor = "red";
        rightColor = "gold";
        leftHight = 0;
        rightHight = sin(fabsf(rollWeb) * 0.017453294) * 140;
      } else if(rollWeb < -tolerance) {
        leftColor = "gold";
        rightColor = "red";
        leftHight = sin(fabsf(rollWeb) * 0.017453294) * 140;
        rightHight = 0;
      } else {
        leftColor = "green";
        rightColor = "green";
        leftHight = 0;
        rightHight = 0;
      }
      rollPct = (rollWeb * dotSensetive) + 50;
      if(rollPct < 10) {
        rollPct = 10;
      } else if(rollPct > 90) {
        rollPct = 90;
      }
      Serial.print(F("roll      : "));Serial.println(roll);
      Serial.print(F("rollWeb   : "));Serial.println(rollWeb);
      Serial.print(F("rollPct   : "));Serial.println(rollPct);
      Serial.print(F("leftHight  : "));Serial.println(leftHight);
      Serial.print(F("rightHight : "));Serial.println(rightHight);
      Serial.print(F("leftColor : "));Serial.println(leftColor);
      Serial.print(F("rightColor: "));Serial.println(rightColor);
    }

    if (isnan(pitch)) {
      Serial.println("Failed to read from MPU6050 sensor!");
    }
    else {
      if(fabsf(fabsf(pitch) - fabsf(pitchWeb)) < 0.15) {
        pitchWeb = ((floatingAvarageCnt -1) * pitchWeb + pitch) / floatingAvarageCnt;
      } else {
        pitchWeb = pitch;
      }
      if(pitchWeb > tolerance) {
        frontColor = "red";
        backColor = "gold";
        frontHight = 0;
        backHight = sin(fabsf(pitchWeb) * 0.017453294) * 210;
      } else if(pitchWeb < -tolerance) {
        frontColor = "gold";
        backColor = "red";
        frontHight = sin(fabsf(pitchWeb) * 0.017453294) * 210;
        backHight = 0;
      } else {
        frontColor = "green";
        backColor = "green";
        frontHight = 0;
        backHight = 0;
      }
      pitchPct = (pitchWeb * dotSensetive) + 60;
      if(pitchPct < 25) {
        pitchPct = 25;
      } else if(pitchPct > 95) {
        pitchPct = 95;
      }
      Serial.println("----");
      Serial.print(F("pitch     : "));Serial.println(pitch);
      Serial.print(F("pitchWeb  : "));Serial.println(pitchWeb);
      Serial.print(F("pitchPct  : "));Serial.println(pitchPct);
      Serial.print(F("frontHight: "));Serial.println(frontHight);
      Serial.print(F("backHight : "));Serial.println(backHight);
      Serial.print(F("frontColor: "));Serial.println(frontColor);
      Serial.print(F("backColor : "));Serial.println(backColor);
    }
  }
}

void handleNotFound()
{ 
//    server.sendHeader("Location", "/",true); //Redirect to our html web page 
//    server.send(302, "text/plane",""); 
}