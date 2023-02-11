# HelloroboArm

![RoboArm image](http://robo.sukelluspaikka.fi/images/RoboArm.jpg)

## Requirements:   

-Arduino MKR 1010 wifi   
-OWI-35 robot arm   
-POLOLU-713 2x1A 4,5-13,5V TB6612FNG Dual Motor Driver Carrier x 3 (or similar)   
-Library:  WiFiNINA, NewPing, ArduinoHttpClient, SPI   


   
Add SSID.h with your ssid information:   

```
#define SSID      "Your gateway id"   
#define KEY       "Your gateway WPA pass"   
```

## Intro

Based on  [OWI-535 building kit](https://owirobot.com/robotic-arm-edge/)   
    with 5 small dc motors and grearing trains, having total 4 degrees on freedom:   
- gripper open and close  (api limits range between -1200...1200, home position 0)  
- wrist up and down 120°  (api limits range between -2800...2800, home position 0)  
- elbow up/down 300°  (api limits range between -7800...7800, home position 0)  
- shoulder rotation 270°  (api limits range between -4000...4000, home position 0) 
- shoulder base 180°  (api limits range between -4800...4800, home position 0)  

Each motor keeps track about it's previous movements trying to avoid a joint running ower it's allowed range of motion.
motion trackking is based on duration in milliseconds moved away from home position (0-pos), where down and ccw are consideres as negative milliseconds.  
after each execution que, the robot arm will return to it's homing position.  


## How to communicate with robot

Robot is sending http get requests to webserver 
```
GET /robot/[device-id]/action/[0-n]
```
Server should responce with one of these actions (text/plain):  
```
/shoulder/rotate/1000
/shoulder/updown/1000	
/elbow/1000	
/wrist/1000	
/pinch/1000	

```

Or with their combinations to run move several joints at the same time:   

```
/shoulder/rotate/1000/elbow/1000/wrist/1000/pinch/1000

```

Once the robot has completed the given seqvence (no more  rows in current project on server), server send special seqvence end -action to robot:     

```
/seq/end

```

 Robot acknowledges the server that it has received this message:   

```
HTTP POST /robot/[device-id]/seq/1/end   
```

Server then checks if there are more seqvences (Projects) in que and push next project into execution (if available).
This protocol is required for keeping http GET -actions to be immutable, but it also allows Robot to enter a manual command mode (receiving manually triggered actions and not switching into next project during manual mode), if robot has /manual -defined in it's api an feature set.  



