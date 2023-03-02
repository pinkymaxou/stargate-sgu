#ifndef _SERVO_H_
#define _SERVO_H_

#define SERVO_PWM2PERCENT(x) ((float)x/20000.0f*100.0f)
#define SERVO_PERCENT2PWM(x) ((float)x/100.0f*20000.0f)

#endif