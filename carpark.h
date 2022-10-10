
#ifndef PTHREAD_H
#define PTHREAD_H
#include <pthread.h>
#endif

typedef struct gate gate_t;
struct gate
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char status;
};

typedef struct LPR LPR_t;
struct LPR
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char plate[6];
};

typedef struct sign sign_t;
struct sign
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char display;
};

typedef struct fire_alarm fire_alarm_t;
struct fire_alarm
{
    short temperature;
    int alarm;
};

typedef struct entrance entrance_t;
struct entrance
{
    LPR_t LPR;
    gate_t gate;
    sign_t sign;
};

typedef struct exit exit_t;
struct exit
{
    LPR_t LPR;
    gate_t gate;
};

typedef struct level level_t;
struct level
{
    LPR_t LPR;
    fire_alarm_t fire_alarm;
};

typedef struct carpark carpark_t;
struct carpark
{
    entrance_t entrance1;
    entrance_t entrance2;
    entrance_t entrance3;
    entrance_t entrance4;
    entrance_t entrance5;
    exit_t exit1;
    exit_t exit2;
    exit_t exit3;
    exit_t exit4;
    exit_t exit5;
    level_t level1;
    level_t level2;
    level_t level3;
    level_t level4;
    level_t level5;
};