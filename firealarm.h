#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "carpark.h"

#include <stdio.h>

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30

typedef struct alarm alarm_t;
struct alarm {
    temperature_t *temperature;
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    gate_t *gate;
    sign_t *sign;
    int *status;
};
//Preconditions: Simulation must be running.
//Parametres: alarm_t
//Monitors the temperature of each level
//Returns: NULL
static void *tempmonitor(void *arg);

//Preconditions: 
//Parametres: alarm_t
//Sets each gate to RAISING when alarm is triggered
//Returns: NULL
static void *open_gate(void *arg);

//Preconditions: 
//Parametres: alarm_t
//Sets each sign to EVACUATE when alarm is triggered
//Returns: NULL
static void *display_evac(void *arg);

//Preconditions: 
//Parameters: temperature_t, int16_t, int
//Reads the value of each temperature sensor and stores in array
static void read_temps(temperature_t *temperature, int16_t *temp, int *count);

//Preconditions: 
//
static void generate_smooth(int16_t *raw_temp, int16_t *smooth_temp, int16_t *median_list, int *raw_count, int *smooth_count);

//Preconditions: 
//
static void detect_fire(int16_t *smooth_temp, int *smooth_count, alarm_t *alarm);

//Preconditions: 
//
static void detect_hardware_failure(int16_t *raw_temp, int raw_count, alarm_t *alarm, int *consecutive_count);
