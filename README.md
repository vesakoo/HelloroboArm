# HelloroboArm

![RoboArm image](http://robo.sukelluspaikka.fi/images/RoboArm.jpg)


Based on  [OWI-535 building kit](https://owirobot.com/robotic-arm-edge/)   
    with 5 small dc motors and grearing trains, having total 4 degrees on freedom:   
- gripper open and close  (api limits range between -1200...1200, home position 0)  
- wrist up and down 120°  (api limits range between -2800...2800, home position 0)  
- elbow up/down 300°  (api limits range between -7800...7800, home position 0)  
- shoulder rotation 270°  (api limits range between -4000...4000, home position 0) 
- shoulder base 180°  (api limits range between -4800...4800, home position 0)  

Each motor keeps track about it's previous movements trying to avoid a joint running ower it's allowed range of motion.
motion trackking is based on duration in milliseconds moved away from upright position (0-pos), where down and ccw are consideres as negative milliseconds.  
after each execution que, the robot arm will return to it's homing position.  

It is a sand box implementation, operating in a single room. It simulates Robot network collaborational functions".

## How to send commands

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

 server sould 

