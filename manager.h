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



// Runs on it's own thread.  The data passed is the carpark shared memory structure: carpark_t
void *generate_GUI(void *arg); 

char *gate_status(char code); void open_gate(); void close_gate(); 

void generate_bill(char *plate);

void generate_car(char *plate, int level);

char find_space();

long long duration_ms(struct timeval start);

void *delete_car(void *arg);

bool string_equal(char *a, char *b);


void *monitor_entry(void *arg); 

void *monitor_exit(void *arg); 

void *monitor_level(void *arg);

void *monitor_gate(void *arg);
