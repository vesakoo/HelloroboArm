#include <Arduino.h>
//#include <NewPing.h>
#include <string.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "SSID.h"
#include <ArduinoHttpClient.h>

#include "ArduinoLowPower.h"

#define SLEEP_10S_AFTER 180000ul
#define SLEEP_15S_AFTER 300000ul
#define SLEEP_30S_AFTER 600000ul
#define SLEEP_60S_AFTER 1800000ul
#define SLEEP_120S_AFTER 3600000ul




//char server[] = "robo.sukelluspaikka.fi";
IPAddress server(192,168,32,87); 
WiFiClient cli;
HttpClient client = HttpClient(cli,server,3002);
int status = WL_IDLE_STATUS;
//char  actions[][];
int actionNum=0;
bool seqEndReported = false;
//last Runtime for deep sleep:
char deviceId [] = "cda-dca-abc";
unsigned long lastRunTime =0l;
unsigned long lastWakeUpTime =0l;


const byte STNDBY=9;
//wrist
const byte a_INA1=8;
const byte a_INA2=7;
const byte a_PWMA=6;
const int aA_max=2800;
const int aA_min=-2800;
//elbow
const byte a_INB1=14;
const byte a_INB2=13;
const byte a_PWMB=10;
const int aB_max=7800;
const int aB_min=-7800;


//shoulder-updown
const byte b_INA1=21;
const byte b_INA2=20;
const byte b_PWMA=19;
const int bA_max=4800;
const int bA_min=-4800;
//shoulder-rotate
const byte b_INB1=16;
const byte b_INB2=17;
const byte b_PWMB=18;
const int bB_max=4000;
const int bB_min=-4000;


//pihti
const byte c_INA1=3;
const byte c_INA2=4;
const byte c_PWMA=5;
const int cA_max=1200;
const int cA_min=-1200;


/*const byte c_INB1=2;
const byte c_INB2=1;
const byte c_PWMB=0;
const int cB_max=1000;
const int cB_min=-1000;*/

//valo
//const byte c_INA1=3;
//const byte c_INA2=4;
//int c_PWMA=5;
const byte SPOT=2;


//int motorpos[]={0,0,0,0,0,0};
enum mposindex {
  PINPWM=0,DURPOS=1,PINM1=2,PINM2=3,BOUNDMIN=4,BOUNDMAX=5
};
enum jointindex {
  WRIST=0,ELBOW=1,SHOULDER=2,SHOULDERROT=3,PINCH=4
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
  //pihti
  //motorLoop(c_INB1,c_INB2,c_PWMB,false);
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
  lastRunTime = lastWakeUpTime+ millis();
}

void multiMotorRun(int durShldRot,int durShld,int durElb,int durWrist,int durPinch){

  durShldRot = checkDur(durShldRot,motorpos[SHOULDERROT][PINPWM]);
  durShld = checkDur(durShld,motorpos[SHOULDER][PINPWM]);
  durElb = checkDur(durElb,motorpos[ELBOW][PINPWM]);
  durWrist = checkDur(durWrist,motorpos[WRIST][PINPWM]);
  durPinch = checkDur(durPinch,motorpos[PINCH][PINPWM]);
  //must have same order than motorMatrix
  long ordered [5] ={durWrist,durElb,durShld,durShldRot,durPinch};
  bool someRunning[5] ={false};
  digitalWrite(STNDBY,HIGH);
  for (int i = 0; i < 5; i++){
    if(ordered[i]<0){
      digitalWrite(motorpos[i][PINM1],LOW);
      digitalWrite(motorpos[i][PINM2],HIGH);
      ordered[i] =  ordered[i] * -1;
    }else{
      digitalWrite(motorpos[i][PINM1],HIGH);
      digitalWrite(motorpos[i][PINM2],LOW); 
    }
    someRunning[i]=ordered[i] > 0;
  }
  long timeStart = millis();
  for (int i = 0; i < 5; i++){
    if(someRunning[i]){
      analogWrite(motorpos[i][PINPWM],150);
    }
  }
  boolean shouldStop = false;
  while(someRunning[0]|| someRunning[1] || someRunning[2] ||someRunning[3]||someRunning[4]){
    for (int i = 0; i < 5; i++){
      if(millis() > timeStart + ordered[i]){
        analogWrite(motorpos[i][PINPWM],0);
        someRunning[i] =false;
      }
    }
  }
  //set all low
  for (int i = 0; i < 5; i++){
    digitalWrite(motorpos[i][PINM1],LOW);
    digitalWrite(motorpos[i][PINM2],LOW); 
  }
  digitalWrite(STNDBY,LOW);
  lastRunTime = lastWakeUpTime + millis();
}

void lightning(bool on){
  if(on){
    analogWrite(SPOT,155);
  }else{
    analogWrite(SPOT,0);
  }
  lastRunTime = lastWakeUpTime + millis(); 
}

void motorsHoming(){
    for (int i = 0; i < 5; i++){
      int mPos = motorpos[i][1] * -1;
      motorRun(motorpos[i][PINM1],motorpos[i][PINM2],motorpos[i][PINPWM],mPos);

    }
    lastRunTime = lastWakeUpTime + millis();
}

//low power interrupt
void dummy() {
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context
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
  //pinMode(c_INB1,OUTPUT);
  //pinMode(c_INB2,OUTPUT);
  pinMode(c_PWMA,OUTPUT);
  //pinMode(c_PWMB,OUTPUT);

  pinMode(STNDBY,OUTPUT);
  setupWifi();
  // LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, dummy, CHANGE);
  //motorRun(a_INA1,)
  //test();
  
}


void loop() {
  WiFi.noLowPowerMode();
  String s = "/robot/" + (String)deviceId +"/action/"+(String)actionNum;
  log(s);
  client.get(s);
  int statusCode = client.responseStatusCode();
  String action = client.responseBody();
  WiFi.lowPowerMode();
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
  ushort runMotors = 0;
  int dur =0,pin1=0,pin2=0,pinpwm=0;
  int shlDur=0,shlRotDur=0,elbDur=0,wristDur=0,pinchDur=0;
  while (ptr) {
    if(strcmp(ptr,"shoulder")==0){
      ptr = strtok(NULL, "/");
      if(strcmp(ptr,"rotate")==0){
        //shoulderrot=true;
        runMotors++;
        pin1=b_INB1;
        pin2=b_INB2;
        pinpwm=b_PWMB;
        ptr = strtok(NULL, "/");
        dur =atoi(ptr);
        shlRotDur=atoi(ptr);
      }else if(strcmp(ptr,"updown")==0){
        //shoulderupdown=true;
        runMotors++;
        pin1=b_INA1;
        pin2=b_INA2;
        pinpwm=b_PWMA;
        ptr = strtok(NULL, "/");
        dur =atoi(ptr);
        shlDur=atoi(ptr);
      }
    }else if(strcmp(ptr,"elbow")==0){
      //elbow=true;
      runMotors++;
      pin1=a_INB1;
      pin2=a_INB2;
      pinpwm=a_PWMB;
      ptr = strtok(NULL, "/");
      dur =atoi(ptr);
      elbDur=atoi(ptr);
    }else if(strcmp(ptr,"wrist")==0){
      //wrist=true;
      runMotors++;
      pin1=a_INA1;
      pin2=a_INA2;
      pinpwm=a_PWMA;
      ptr = strtok(NULL, "/");
      dur =atoi(ptr);
      wristDur=atoi(ptr);
    }else if (strcmp(ptr,"pinch")==0){
      //pinch=true;
      runMotors++;
      pin1=c_INA1;
      pin2=c_INA2;
      pinpwm=c_PWMA;
      ptr = strtok(NULL, "/");
      dur =atoi(ptr);
      pinchDur=atoi(ptr);
    }else if (strcmp(ptr,"light")==0){
      ptr = strtok(NULL, "/");
      lightning(strcmp(ptr,"on") == 0); 
    }else if(strcmp(ptr,"homepos")==0){
      motorsHoming();
    }
    ptr = strtok(NULL, "/"); 
    //break;
  }
  if(pinpwm != 0){
    if(runMotors>1){
      multiMotorRun(shlRotDur,shlDur,elbDur,wristDur,pinchDur);
    }else{
      motorRun(pin1,pin2,pinpwm,dur);
    }
  }
  if(action.endsWith("/seq/end")){
    log("sequence end catched!");
    if(!seqEndReported){
        postSeqEnd();
        seqEndReported = true;
        actionNum =0;
    }
  }else{
    action ="";
    //send = true;
    seqEndReported =false;
    actionNum++; 
    //actionNum = manualMode?actionNum:0;
  }

    //if seq/end many times, goto sleep for a while
    unsigned long now = millis();
    if(now<lastWakeUpTime){
      now = lastWakeUpTime+now; //now will go out of bounds in 46 days...
    }
    if (now > lastRunTime + SLEEP_120S_AFTER ){
     log("LOW POWER 120S");
     lastWakeUpTime =now +120000;
     LowPower.sleep(120000);
     lastRunTime = now - SLEEP_120S_AFTER; //so that lastRuntime dont go out of bounds
    }
    else if(now > lastRunTime + SLEEP_60S_AFTER  ){
      log("LOW POWER 60S");
      lastWakeUpTime =now +60000;
      LowPower.sleep(60000);
    }
    else if(now > lastRunTime + SLEEP_30S_AFTER  ){
      log("LOW POWER 30S");
      lastWakeUpTime =now + 30000;
      LowPower.sleep(30000);
    }
    else if(now > lastRunTime + SLEEP_15S_AFTER  ){
      log("LOW POWER 15S");
      lastWakeUpTime =now +15000;
      LowPower.sleep(15000);
    }
    else if(now > lastRunTime + SLEEP_10S_AFTER  ){ //after 3 mins of idling sleep a second
      log("LOW POWER 10S");
      lastWakeUpTime =now + 10000;
      LowPower.sleep(10000);

    }
  

}
