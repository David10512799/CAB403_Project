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

//Reads the plate.txt file and returns the number of plates in the file.
//Preconditions: None
//Postconditions: update plate_count with number of plates in plates.txt
int get_plate_count();

//Initialises the carpark in shared memory
//Preconditions: None.
//Postconditions: true
bool init_carpark(shared_carpark_t* carpark);

//Initialises the carparks values in shared memory
//Preconditions: Carpark shared memory has been initialised
//Postconditions: Initialises carpark values in shared memory
void init_carpark_values(carpark_t* park);

//Simulates the life-cycle of both a valid license plate and invalid license plate.
//Preconditions: Shared memory has been initialised
//Postconditions: returns NULL 
void *sim_car(void *arg);

//Creates a new thread for each car waiting between 1 and 100ms
//Preconditions: Shared memory has been initialised.
//Postconditions: Thread is created with new car and life-cycle is simulated
void start_car_simulation(char** plate_registry);

// Add a new node to the start of the linked list containing the plate value provided.
//Preconditions: None.
//Postconditions: Returns the new head of the the linked list
node_t *node_add(node_t *head, char *plate);

// Find the node containing the plate provided.
//Preconditions: None.
//Postconditions: Returns the node that contains the plate if it exists, or NULL if it doesn't.
node_t *node_find_name(node_t *head, char *plate);

// Find a node containing the plate provided within an array of linked lists. 
//Preconditions: None.
//Postconditions: Returns the node that contains the plate if it exists, or NULL if it doesn't.
node_t *node_find_name_array(node_t **node_array, char *plate, int array_len);

// Delete a node from the linked list if it exists.
//Preconditions: None.
//Postconditions: Returns the new head of the linked list.
node_t *node_delete(node_t *head, char *plate);

//Monitors each gate and is triggered when the manager sets a gate to RAISING or LOWERING
//Preconditions: Shared memory has been initialised.
//Postconditions: Returns NULL
void *monitor_gate(void *arg);

//Simulates the temperature within each level of the carpark
//Preconditions: Shared memory is initialised.
//Postconditions: returns NULL once alarms are triggered
void *temp_sim(void *arg);

//Generate normal temperature values
//Preconditions: None.
//Postconditions: New temperature is returned.
int normal_temp(int current_temp);