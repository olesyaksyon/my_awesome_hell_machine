#ifndef _SONAR_H_
#define _SONAR_H_

#define CLASS_NAME  "sonar"
#define DEVICE_BUS "sonar"
#define DEVICE_NAME "sonar"
#define DEV_MINOR 0

#define MAX_OPENED_FILES 256

struct sonar_file {
    u16 id;
    u16 read_pos;
    u32 read_value;
    u8 read_pending;
};

typedef struct sonar_file sonar_file_t;

#endif
