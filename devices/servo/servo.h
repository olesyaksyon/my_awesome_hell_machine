#ifndef _SERVO_H_
#define _SERVO_H_

#define CLASS_NAME  "servo"
#define DEVICE_BUS "servo"
#define DEVICE_NAME "servo"
#define DEV_MINOR 0

#define PERIOD 20000

struct servo_file {
    u16 read_pos;
    char buffer[4];
};

typedef struct servo_file servo_file_t;

#endif

