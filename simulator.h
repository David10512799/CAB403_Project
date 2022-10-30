#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "carpark.h"
#include "carlist.h"

extern int errno;

typedef struct node node_t;
struct node
{
    char *plate;
    node_t *next;
};

node_t *entry_list[ENTRIES];
node_t *exit_list[EXITS];

//Car Simulation

//Preconditions: 
//Reads the plate.txt file and returns the number of plates in the file.
//Returns: plate_count
int get_plate_count();

//Preconditions: 
//Initialises the carpark in shared memory
//Returns: true
bool init_carpark(shared_carpark_t* carpark);

//Preconditions: 
//Parameters: carpark_t
//Initialises the carparks values in shared memory
void init_carpark_values(carpark_t* park);

//Preconditions: 
//Parameters: char **plate_registry
//Simulates the life-cycle of both a valid license plate and invalid license plate.
//Returns: NULL
void *sim_car(void *arg);

//Preconditions: 
//Parameters: char **plate_registry
//Creates a new thread for each car waiting between 1 and 100ms
void start_car_simulation(char** plate_registry);

//Preconditions: 
node_t *node_add(node_t *head, char *plate);

//Preconditions: 
node_t *node_find_name(node_t *head, char *plate);

//Preconditions: 
node_t *node_find_name_array(node_t **node_array, char *plate, int array_len);

//Preconditions: 
node_t *node_delete(node_t *head, char *plate);

//Preconditions: Shared memory has been initialised.
//Monitors each gate and is triggered when the manager sets a gate to RAISING or LOWERING
void *monitor_gate(void *arg);

//Preconditions: 
//Simulates the temperature within each level of the carpark
//Returns: NULL
void *temp_sim(void *arg);

//Preconditions: 
//Generate normal temperature values
//Returns: New temperature
int normal_temp(int current_temp);