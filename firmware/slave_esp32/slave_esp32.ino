#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Safety System Industrial"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ✅ WIFI
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ✅ TELEGRAM
const char* botToken = "YOUR_TELEGRAM_BOT_TOKEN";
const char* chatID   = "YOUR_TELEGRAM_CHAT_ID";

// ✅ RELAY
#define RELAY 5

// ✅ OLED
Adafruit_SSD1306 display(128,64,&Wire,-1);

// ✅ VARIABLES
float temp=0;
int gas=0;
int light=0;
int intr=0;
String status="WAIT";
String modeName="NORMAL";
String alertState="ON";

// ✅ RELAY STATE
bool relayManual=false;
int relayState=LOW;

//--------------------------------------------------
// ✅ TELEGRAM (SIMPLE & STABLE)
void sendTelegram(String msg){
  if(WiFi.status()==WL_CONNECTED){
    HTTPClient http;

    String url="https://api.telegram.org/bot"+String(botToken)+"/sendMessage";

    http.begin(url);
    http.addHeader("Content-Type","application/x-www-form-urlencoded");

    String data="chat_id="+String(chatID)+"&text="+msg;

    http.POST(data);
    http.end();
  }
}

//--------------------------------------------------
// ✅ BLYNK RELAY CONTROL
BLYNK_WRITE(V5){
  relayManual = true;
  relayState = param.asInt();
  digitalWrite(RELAY, relayState);
}

//--------------------------------------------------
void setup(){

  Serial.begin(115200);

  // ✅ FIXED UART
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  pinMode(RELAY, OUTPUT);

  // ✅ OLED INIT
  Wire.begin(21,22);
  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println("OLED FAIL ❌");
    while(true);
  }

  display.setTextColor(WHITE);

  // ✅ WIFI + BLYNK
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  Serial.println("System Ready ✅");

  sendTelegram("System Started");
}

//--------------------------------------------------
void loop(){

  Blynk.run();   // ✅ IMPORTANT

  // ✅ UART RECEIVE
  if(Serial2.available()){

    String d = Serial2.readStringUntil('\n');
    Serial.println("RAW: "+d);

    int a=d.indexOf(',');
    int b=d.indexOf(',',a+1);
    int c=d.indexOf(',',b+1);
    int d1=d.indexOf(',',c+1);
    int e=d.indexOf(',',d1+1);
    int f=d.indexOf(',',e+1);

    // ✅ SAFE PARSE (prevents crash)
    if(a>0 && f>0){

      temp  = d.substring(0,a).toFloat();
      gas   = d.substring(a+1,b).toInt();
      light = d.substring(b+1,c).toInt();
      intr  = d.substring(c+1,d1).toInt();
      status= d.substring(d1+1,e);

      int mode  = d.substring(e+1,f).toInt();
      int alert = d.substring(f+1).toInt();

      modeName   = (mode==0)?"NORMAL":(mode==1)?"POWER":"EMERGENCY";
      alertState = alert ? "ON":"OFF";

      //--------------------------------------------------
      // ✅ RELAY AUTO + MANUAL
      if(!relayManual){
        relayState = (status=="FIRE");
        digitalWrite(RELAY, relayState);
      }

      // RESET AUTO WHEN SAFE
      if(status=="SAFE") relayManual=false;

      //--------------------------------------------------
      // ✅ TELEGRAM (SIMPLE ✅)
      String msg =
        "Mode: " + modeName +
        "\nStatus: " + status +
        "\nTemp: " + String(temp) +
        "\nGas: " + String(gas) +
        "\nIntrusion: " + String(intr);

      sendTelegram(msg);

      //--------------------------------------------------
      // ✅ BLYNK UPDATE
      Blynk.virtualWrite(V0,temp);
      Blynk.virtualWrite(V1,gas);
      Blynk.virtualWrite(V2,light);
      Blynk.virtualWrite(V3,intr);
      Blynk.virtualWrite(V4,status);
      Blynk.virtualWrite(V5,relayState);

      //--------------------------------------------------
      // ✅ SERIAL DEBUG (FULL)
      Serial.println("------ SLAVE SYSTEM ------");
      Serial.print("Mode     : "); Serial.println(modeName);
      Serial.print("Alert    : "); Serial.println(alertState);
      Serial.print("Temp     : "); Serial.println(temp);
      Serial.print("Gas      : "); Serial.println(gas);
      Serial.print("Light    : "); Serial.println(light);
      Serial.print("Intruder : "); Serial.println(intr);
      Serial.print("Status   : "); Serial.println(status);
      Serial.print("Relay    : "); Serial.println(relayState?"ON":"OFF");
      Serial.print("Control  : "); Serial.println(relayManual?"MANUAL":"AUTO");
      Serial.println("---------------------------\n");

      //--------------------------------------------------
      // ✅ OLED (WORKING LAYOUT ✅)
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(1);

      display.println("SAFE SYSTEM");
      display.println("-----------");

      display.print("Mode: "); display.println(modeName);
      display.print("Alert: "); display.println(alertState);

      display.print("T:"); display.print(temp);
      display.print(" G:"); display.println(gas);

      display.print("I:"); display.print(intr);
      display.print(" S:"); display.println(status);

      display.print("R:"); display.print(relayState?"ON":"OFF");
      display.print(" M:"); display.println(relayManual?"M":"A");

      display.display();
    }
  }

  delay(200);
}