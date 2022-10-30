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

//The data passed is the carpark shared memory structure: carpark_t
//Preconditions: Shared Memory must be initialised.
//Postconditions: Builds the manager GUI in the terminal
void *generate_GUI(void *arg); 

//Calculates cost for each car entering the car park.
//Preconditions: Shared Memory must be initialised.
//Postconditions: Updates billing.txt file with updated cost.
void generate_bill(char *plate);

//
//Preconditions: Shared Memory must be initialised.
//Postconditions: 
void generate_car(char *plate, int level);

//Finds the level to send each car, starts from level 1 and fills until full, moves onto next level.
//Preconditions: 
//Postconditions:The level in which the simulator is to send the car. 
char find_space();

//Calculates the time each car has spent in the carpark
//Preconditions: Car must be generated.
//Postconditions: Time in carpark in ms
long long duration_ms(struct timeval start);

//Monitors the gates and waits for signal to switch to RAISING or LOWERING
//Preconditions: Shared Memory must be initialised.
//Postconditions: The meaning of the gate status characters stored in shared memory
char *gate_status(char code); void open_gate(); void close_gate(); 

//Monitors the entry LPR's 
//Preconditions: 
//Postconditions: NULL
void *monitor_entry(void *arg); 

//Monitors the exit LPR's
//Preconditions: 
//Postconditions: NULL
void *monitor_exit(void *arg); 

//Monitors each level LPR
//Preconditions: 
//Postconditions: NULL
void *monitor_level(void *arg);

//Monitors the position of each gate.
//Preconditions: 
//Postconditions: NULL
void *monitor_gate(void *arg);
