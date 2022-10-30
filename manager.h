#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "carpark.h"
#include "carlist.h"

typedef struct level_LPR_monitor level_LPR_monitor_t;
struct level_LPR_monitor
{
    int id;
    LPR_t *level_LPR;
};


//Preconditions: Shared Memory must be initialised.
//Runs on it's own thread.
//The data passed is the carpark shared memory structure: carpark_t
//Builds the manager GUI in the terminal
void *generate_GUI(void *arg); 

//Preconditions: Shared Memory must be initialised.
//Calculates cost for each car entering the car park.
//Returns: Updates billing.txt file with updated cost.
void generate_bill(char *plate);

//Preconditions: Shared Memory must be initialised.
void generate_car(char *plate, int level);

//Preconditions: 
//Finds the level to send each car, starts from level 1 and fills until full, moves onto next level.
//Returns: The level in which the simulator is to send the car. 
char find_space();

//Preconditions: 
//Calculates the time each car has spent in the carpark
//Returns: Time in carpark in ms
long long duration_ms(struct timeval start);

//Preconditions: Shared Memory must be initialised.
//
//Returns: The meaning of the gate status characters stored in shared memory
char *gate_status(char code); void open_gate(); void close_gate(); 

//Preconditions: 
//Monitors the entry LPR's 
//Returns: NULL
void *monitor_entry(void *arg); 

//Preconditions: 
//Monitors the exit LPR's
//Returns: NULL
void *monitor_exit(void *arg); 

//Preconditions: 
//Monitors each level LPR
//Returns: NULL
void *monitor_level(void *arg);

//Preconditions: 
//Monitors the position of each gate.
//Returns: NULL
void *monitor_gate(void *arg);
