#ifndef CARPARK_H
#define CARPARK_H
#include <pthread.h> 
#include <stdbool.h> 
#include <unistd.h>
#include <stdlib.h>
#endif

#define SHARE_NAME "PARKING"
#define NUM_ENTRIES 5
#define NUM_EXITS 5
#define NUM_LEVELS 5
#define CARS_PER_LEVEL 20
#define EMPTY_LPR ""
#define RAISING 'R'
#define LOWERING 'L'
#define CLOSED 'C'
#define OPEN 'O'
#define DENIED 'X'
#define FULL 'F'



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
    char *plate;
};

typedef struct sign sign_t;
struct sign
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char display;
};

typedef struct temperature temperature_t;
struct temperature
{
    int16_t sensor;
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
    temperature_t temperature;
};


typedef struct carpark carpark_t;
struct carpark
{
    entrance_t entrance[5];
    exit_t exit[5];
    level_t level[5];
};

typedef struct shared_carpark shared_carpark_t;
struct shared_carpark
{
    const char* name;
    int fd;
    carpark_t* data;
};


bool monitor_alarms(level_t *level)
{
    while(level->temperature.alarm == 0)
    {
        usleep(500);
    }
    return true; 
}

#define TIMEX 100 // Time multiplier for timings to slow down simulation - set to 1 for specified timing

void ms_pause(int time)
{
    usleep(TIMEX * time * 1000);
}