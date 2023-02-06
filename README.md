# HelloroboArm

Based on  [OWI-535 building kit](https://owirobot.com/robotic-arm-edge/)   
    with 5 small dc motors and grearing trains, having total 4 degrees on freedom:   
- gripper open and close   
- wrist up and down 120°  
- elbow up/down 300°  
- shoulder rotation 270°  
- shoulder base 180°  
- gripper spot light  
- horizontal reach  32 cm  
- vertical reach 38 cm  
- 100g lifting capacity  

Each motor keeps track about it's previous movements trying to avoid a joint running ower it's allowed range of motion.
motion trackking is based on duration in milliseconds moved away from upright position (0-pos), where down and ccw are consideres as negative milliseconds.  
after each execution que, the robot arm will return to it's homing position.  

It is a sand box implementation, operating in a single room. It simulates Robot network collaborational functions",
