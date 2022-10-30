#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "carpark.h"

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

//Monitors the temperature of each level
//Preconditions: Simulation must be running.
//Postconditions: NULL
static void *tempmonitor(void *arg);

//Sets each gate to RAISING when alarm is triggered
//Preconditions: 
//Postconditions: NULL
static void *open_gate(void *arg);

//Sets each sign to EVACUATE when alarm is triggered
//Preconditions: 
//Postconditions: NULL
static void *display_evac(void *arg);

//Reads the value of each temperature sensor and stores in array
//Preconditions: Simulation must be running
//Postconditions: Stores the temperatures in an array
static void read_temps(temperature_t *temperature, int16_t *temp, int *count);

//Calculates the median of the past 5 raw temp values and stores in smooth_temp
//Preconditions: Simulation must be running
//Postconditions: Stores the smoothed temperatures into an array
static void generate_smooth(int16_t *raw_temp, int16_t *smooth_temp, int16_t *median_list, int *raw_count, int *smooth_count);

//Detects when a fire has been simulated
//Preconditions: Simulation is running
//Postconditions: Alarm->status = 1
static void detect_fire(int16_t *smooth_temp, int *smooth_count, alarm_t *alarm);

//Detects if there is a hardware error with the temperature sensors
//Preconditions: Simulation is running
//Postconditions: Alarm->status = 1
static void detect_hardware_failure(int16_t *raw_temp, int raw_count, alarm_t *alarm, int *consecutive_count);
