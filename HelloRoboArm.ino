#include <Arduino.h>
//#include <NewPing.h>
#include <string.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "SSID.h"
#include <ArduinoHttpClient.h>

//char server[] = "robo.sukelluspaikka.fi";

IPAddress server(192,168,32,87); 
char deviceId [] = "cda-dca-abc";
WiFiClient cli;
HttpClient client = HttpClient(cli,server,3002);
int status = WL_IDLE_STATUS;
//char  actions[][];
int actionNum=0;
bool seqEndReported = false;


const byte STNDBY=9;
//wrist
const byte a_INA1=8;
const byte a_INA2=7;
const byte a_PWMA=6;
const int aA_max=1000;
const int aA_min=-1000;
//elbow
const byte a_INB1=14;
const byte a_INB2=13;
const byte a_PWMB=10;
const int aB_max=1000;
const int aB_min=-1000;


//shoulder-updown
const byte b_INA1=21;
const byte b_INA2=20;
const byte b_PWMA=19;
const int bA_max=1000;
const int bA_min=-1000;
//shoulder-rotate
const byte b_INB1=16;
const byte b_INB2=17;
const byte b_PWMB=18;
const int bB_max=1000;
const int bB_min=-1000;


//pihti
const byte c_INA1=3;
const byte c_INA2=4;
const byte c_PWMA=5;
const int cA_max=1000;
const int cA_min=-1000;

//valo
const byte c_INB1=2;
const byte c_INB2=1;
int c_PWMB=0;


//int motorpos[]={0,0,0,0,0,0};
enum mposindex {
  PINPWM=0,DURPOS=1,PINM1=2,PINM2=3,BOUNDMIN=4,BOUNDMAX=5
};
int motorpos[5][6] ={
  {a_PWMA,0,a_INA1,a_INA2,aA_min,aA_max},
  {a_PWMB,0,a_INB1,a_INB2,aB_min,aB_max},
  {b_PWMA,0,b_INA1,b_INA2,bA_min,bA_max},
  {b_PWMB,0,b_INB1,b_INB2,bB_min,bB_max},
  {c_PWMA,0,c_INA1,c_INA2,cA_min,cA_max}
}; 
int motorBounds[5][3] ={
  {a_PWMA,aA_min,aA_max},
  {a_PWMB,aB_min,aB_max},
  {b_PWMA,bA_min,bA_max},
  {b_PWMB,bB_min,bB_max},
  {c_PWMA,cA_min,cA_max}
}; 

//old loop for testing
void motorLoop(int in1,int in2,int pwa,bool longUp){
  digitalWrite(STNDBY,HIGH);
  Serial.println("Start");
  digitalWrite(in1,HIGH);
  digitalWrite(in2,LOW);
  analogWrite(pwa,150);
  delay(1000);
  Serial.println("Stop");
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  delay(1000);
  Serial.println("switch");
  digitalWrite(in1,LOW);
  digitalWrite(in2,HIGH);
  if(longUp){
    delay(1200);
  }else{
    delay(1000);
  }
  Serial.println("stop");
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  analogWrite(pwa,0);
  delay(1000);
}

void test(){
  digitalWrite(STNDBY,HIGH);
  //ranne
  motorLoop(a_INA1,a_INA2,a_PWMA,true);
  //kyynärpää
  motorLoop(a_INB1,a_INB2,a_PWMB,true);
  //lantio
  motorLoop(b_INA1,b_INA2,b_PWMA,true);
  //lantio-rot
  motorLoop(b_INB1,b_INB2,b_PWMB,false);
  //pihti
  motorLoop(c_INA1,c_INA2,c_PWMA,false);
  //valo
  motorLoop(c_INB1,c_INB2,c_PWMB,false);
}

void setupWifi(){
  while (status != WL_CONNECTED) {
    status = WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(10000);
  }
  client.setTimeout(3000);
  printWiFiStatus();
}

void getActions(){
}

void postSeqEnd(){
  String contentType = "application/json";
  String postData = "{\"seq\":\"end\"}";
  char str[80];
  sprintf(str, "/robot/%s/seq/1/end", deviceId);
  puts(str);
  client.post(str,contentType, postData);
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
}



void printWiFiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void log(String s){
  Serial.println(s);
}

//limit movement and keep estimated track on position
int checkDur(int dur, byte pwmpin){
  int durToRet=dur;
  for (short i = 0; i < 5; i++){
    if(motorpos[i][0]==pwmpin){
      int newTmpPos = motorpos[i][1] + dur;
      if(newTmpPos < motorBounds[i][1]){
        durToRet = motorBounds[i][1] - motorpos[i][1]; 
        log("reducing duration");
      }
      else if(newTmpPos > motorBounds[i][2]){
        durToRet = motorBounds[i][2]-motorpos[i][1];
        log("reducing duration");
      }
      motorpos[i][1] = motorpos[i][1] +durToRet;
      log("motorpos [" + (String)i+"][1]: " +(String)motorpos[i][1] );
    }  
  }
  return durToRet;
}

void motorRun( byte pin1,byte pin2,byte pwmpin, int dur){
  
  dur = checkDur(dur,pwmpin); //limitter
  log("motorRun (pin1=" +(String)((int)pin1) +",pin2="+(String)((int)pin2)+",pwmpin="+(String)((int)pwmpin)+",dur="+(String)dur);
  digitalWrite(STNDBY,HIGH);
  digitalWrite(pin1,dur>0?HIGH:LOW);
  digitalWrite(pin2,dur>0?LOW:HIGH);
  analogWrite(pwmpin,150);
  delay(dur>0?dur:-1*dur);
  analogWrite(pwmpin,0);
  digitalWrite(pin1,LOW);
  digitalWrite(pin2,LOW);
  digitalWrite(STNDBY,LOW);
}
void lightning(bool on){

}

void motorsHoming(){
    for (int i = 0; i < 5; i++){
      int mPos = motorpos[i][1] * -1;
      motorRun(motorpos[i][2],motorpos[i][3],motorpos[i][0],mPos);

    }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(a_INA1,OUTPUT);
  pinMode(a_INA2,OUTPUT);
  pinMode(a_INB1,OUTPUT);
  pinMode(a_INB2,OUTPUT);
  pinMode(a_PWMA,OUTPUT);
  pinMode(a_PWMB,OUTPUT);

  pinMode(b_INA1,OUTPUT);
  pinMode(b_INA2,OUTPUT);
  pinMode(b_INB1,OUTPUT);
  pinMode(b_INB2,OUTPUT);
  pinMode(b_PWMA,OUTPUT);
  pinMode(b_PWMB,OUTPUT);

  pinMode(c_INA1,OUTPUT);
  pinMode(c_INA2,OUTPUT);
  pinMode(c_INB1,OUTPUT);
  pinMode(c_INB2,OUTPUT);
  pinMode(c_PWMA,OUTPUT);
  pinMode(c_PWMB,OUTPUT);

  pinMode(STNDBY,OUTPUT);
  setupWifi();
  //motorRun(a_INA1,)
  //test();
  
}


void loop() {

  String s = "/robot/" + (String)deviceId +"/action/"+(String)actionNum;
  log(s);
  client.get(s);
  int statusCode = client.responseStatusCode();
  String action = client.responseBody();
  //send =false;
  Serial.println("<Loop Action is:" +action +">");
  int str_len = action.length() + 1; 
  char char_array[str_len]; 
  action.toCharArray(char_array, str_len);
  char* ptr = strtok(char_array, "/");

  //bool shoulderupdown = false;
  //bool shoulderrot = false;
  //bool elbow = false;
  //bool  wrist = false;
  //bool pinch = false;
  //bool unpinch = false;
  bool runMotors = false;
  int dur =0,pin1=0,pin2=0,pinpwm=0;

  while (ptr) {
    if(strcmp(ptr,"shoulder")==0){
      ptr = strtok(NULL, "/");
      if(strcmp(ptr,"rotate")==0){
        //shoulderrot=true;
        runMotors=true;
        pin1=b_INB1;
        pin2=b_INB2;
        pinpwm=b_PWMB;
      }else if(strcmp(ptr,"updown")==0){
        //shoulderupdown=true;
        runMotors=true;
        pin1=b_INA1;
        pin2=b_INA2;
        pinpwm=b_PWMA;
      }
    }else if(strcmp(ptr,"elbow")==0){
      //elbow=true;
      runMotors=true;
      pin1=a_INB1;
      pin2=a_INB2;
      pinpwm=a_PWMB;

    }else if(strcmp(ptr,"wrist")==0){
      //wrist=true;
      runMotors=true;
      pin1=a_INA1;
      pin2=a_INA2;
      pinpwm=a_PWMA;

    }else if (strcmp(ptr,"pinch")==0){
      //pinch=true;
      runMotors=true;
      pin1=c_INA1;
      pin2=c_INA2;
      pinpwm=c_PWMA;
    }
    ptr = strtok(NULL, "/");
    dur =atoi(ptr);
    break;
  }
  if(pinpwm != 0){
    motorRun(pin1,pin2,pinpwm,dur);
  }
  if(action.endsWith("/seq/end")){
    log("sequence end catched!");
    if(!seqEndReported){
        postSeqEnd();
        //manualMode=false;
        seqEndReported = true;
        actionNum =0;
      //}
    }
  }else{
    action ="";
    //send = true;
    seqEndReported =false;
    actionNum++; 
    //actionNum = manualMode?actionNum:0;
  }


  

}
