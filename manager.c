#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
 
#include "carpark.h"
#include "carlist.h"


size_t buckets = 40;
htab_t verified_cars;

pthread_mutex_t space_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t revenue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hash_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t billing_lock = PTHREAD_MUTEX_INITIALIZER;
volatile int freespaces[NUM_LEVELS];
volatile float total_revenue = 0.00;

typedef struct level_LPR_monitor level_LPR_monitor_t;
struct level_LPR_monitor
{
    int id;
    LPR_t *level_LPR;
};

//Function declarations
void generate_GUI(carpark_t *data); 
char *gate_status(char code); void open_gate(); void close_gate(); 
void *generate_bill(void *arg);
void generate_car(char *plate, int level);
char find_space();
long long duration_ms(struct timeval start);
void pause_for(int time);
void *delete_car(void *arg);

// Monitors
void *monitor_entry(void *arg); void *monitor_exit(void *arg); void *monitor_level(void *arg);
void *monitor_gate(void *arg);




int main(int argc, char **argv){

    // Get shared memory object
    shared_carpark_t carpark;
    get_carpark(&carpark);

    // Initialise hashtable and insert plates from "plates.txt"
    
    if (!htab_init(&verified_cars, buckets))
    {
        printf("failed to initialise hash table\n");
        return EXIT_FAILURE;
    }

    htab_insert_plates(&verified_cars);

    // Initialise the list of cars with all spaces remaining
    for( int i = 0; i < NUM_LEVELS; i++){
        freespaces[i] = CARS_PER_LEVEL;
    }

    // Begin monitoring Entrance License Plate Readers to detect Cars and Boom Gates put into OPEN status
    for( int i = 0; i < NUM_ENTRIES; i++){
        pthread_t entry_LPR, entry_gate;
        pthread_create(&entry_LPR, NULL, monitor_entry, &carpark.data->entrance[i]); //needs access to LPR and gate
        pthread_create(&entry_gate, NULL, monitor_gate, &carpark.data->exit[i].gate);
    }

    // Begin monitoring Level License Plate Readers to detect Cars
    for( int i = 0; i < NUM_LEVELS; i++){
        // initialise struct for passing to thread so it knows which level to change the car to
        level_LPR_monitor_t monitor;
        monitor.level_LPR = &carpark.data->level[i].LPR;
        monitor.id = i;

        pthread_t level_LPR;
        pthread_create(&level_LPR, NULL, monitor_level, &monitor);
    }
    
    // Begin monitoring Exit License Plate Readers to detect Cars
    for ( int i = 0; i < NUM_EXITS; i++)
    {
        pthread_t exit_LPR, exit_gate;
        pthread_create(&exit_LPR, NULL, monitor_exit, &carpark.data->exit[i]); // needs access to LPR and gate
        pthread_create(&exit_gate, NULL, monitor_gate, &carpark.data->exit[i].gate);
    }


    while (true) sleep(10);

    return EXIT_SUCCESS;

}

#define TIMEX 100 // Time multiplier for timings to slow down simulation - set to 1 for specified timing

void pause_for(int time)
{
    usleep(TIMEX * time);
}

void *monitor_gate(void *arg)
{
    gate_t *gate = (gate_t *)arg;

    for(;;)
    {
        //lock gate mutex and wait for signal
        pthread_mutex_lock(&gate->mutex);
        while(gate->status != OPEN)
            pthread_cond_wait(&gate->condition, &gate->mutex);

        if (gate->status == OPEN) // Not sure if this line is necessary?
        {    
        // delay for 20ms
        pause_for(20);
        // Set the gate status to Lowering
        gate->status = LOWERING;
        }
        // signal condition variable and unlock mutex
        pthread_cond_signal(&gate->condition);
        pthread_mutex_unlock(&gate->mutex);
    }

}

void *monitor_level(void *arg)
{
    level_LPR_monitor_t *lpr = (level_LPR_monitor_t *)arg;

    for(;;)
    {
        // lock lpr mutex and wait for signal
        pthread_mutex_lock(&lpr->level_LPR->mutex);
        while (lpr->level_LPR->plate == EMPTY_LPR)
            pthread_cond_wait(&lpr->level_LPR->condition, &lpr->level_LPR->mutex);

        
        int index = lpr->id;
        // Corrected from indexing value to actual level
        int level = index + 1; 
        char *plate = lpr->level_LPR->plate;

        // get pointer to car
        car_t *car = htab_find(&verified_cars, plate);

        // check car is supposed to be on level that there is room on that level and update values if not
        if ( (car->current_level != level) && ( freespaces[index] != 0 ) )
        {
            // Decrement number of free spaces on level
            freespaces[index]--;
            // Correct for indexing
            int current_level = car->current_level - 1; 
            // Increment number of free spaces on level
            freespaces[current_level]++;
            // Edit current level to new value
            car->current_level = level;
        }

        // unlock mutex
        pthread_mutex_unlock(&lpr->level_LPR->mutex);
    }
}

void *monitor_exit(void *arg)
{
    exit_t *exit = (exit_t *)arg;

    for(;;)
    {
        // lock lpr mutex and wait for signal
        pthread_mutex_lock(&exit->LPR.mutex);
        while (exit->LPR.plate == EMPTY_LPR)
            pthread_cond_wait(&exit->LPR.condition, &exit->LPR.mutex);

        // Read plate
        char *plate = exit->LPR.plate;

        // Bill car
        pthread_t bill;
        pthread_create(&bill, NULL, generate_bill, plate);
        pthread_join(bill, NULL);
        // Remove car from carpark
        pthread_t car;
        pthread_create(&car, NULL, delete_car, plate);

        // Lock gate mutex
        pthread_mutex_lock(&exit->gate.mutex);
        // Set gate status to RAISING
        exit->gate.status = RAISING;
        // Signal condition signal and unlock mutex
        pthread_cond_signal(&exit->gate.condition);
        pthread_mutex_unlock(&exit->gate.mutex);

        // unlock lpr mutex and reset LPR
        exit->LPR.plate = EMPTY_LPR;
        pthread_mutex_unlock(&exit->LPR.mutex);
    }
}

void *monitor_entry(void *arg)
{
    entrance_t *entry = (entrance_t *)arg;

    for (;;)
    {
        // lock lpr mutex and wait for signal
        pthread_mutex_lock(&entry->LPR.mutex);
        while (entry->LPR.plate == EMPTY_LPR)
            pthread_cond_wait(&entry->LPR.condition, &entry->LPR.mutex);
        
        // Read plate and determine if allowed in.
        char *plate = entry->LPR.plate;
        char space;

        // Acquire hash and space locks out here so that car can be generated without losing its spot
        pthread_mutex_lock(&hash_lock);
        pthread_mutex_lock(&space_lock);

        if ( htab_search_plate(&verified_cars, plate) )
            space = find_space();
        else
            space = DENIED;


        // Set gate to raising if level = 1 - 5
        int level = (int)space - 48;
        if (0 < level && level < 6)
        {
            generate_car(plate, level);

            pthread_mutex_lock(&entry->gate.mutex);

            entry->gate.status = RAISING;

            pthread_cond_signal(&entry->gate.condition);
            pthread_mutex_unlock(&entry->gate.mutex);
        }

        pthread_mutex_unlock(&hash_lock);
        pthread_mutex_unlock(&space_lock);

        // Display entry status on sign
        pthread_mutex_lock(&entry->sign.mutex);

        entry->sign.display = level;

        pthread_cond_signal(&entry->sign.condition);
        pthread_mutex_unlock(&entry->sign.mutex);

        // reset lpr unlock lpr mutex
        entry->LPR.plate = EMPTY_LPR;
        pthread_mutex_unlock(&entry->LPR.mutex);
    }

}

// Find the level with the most spots remaining or return FULL
char find_space()
{
    char retVal = FULL;
    int level = 0;

    for( int i = 1; i < NUM_LEVELS; i++){
        level = freespaces[i] > freespaces[i-1] ? i + 1 : i;
    }

    if (level != 0)
        retVal = (char)level;

    return retVal;
}

void generate_car(char *plate, int level)
{
    car_t new_car;
    sprintf(new_car.plate, plate);
    new_car.current_level = level;

    struct timeval entry_time;
    gettimeofday(&new_car.entry_time, NULL);

    add_car(&verified_cars, &new_car);
}

void *delete_car(void *arg)
{
    char *plate = (char *)arg;
    remove_car(&verified_cars, plate);
}

void *generate_bill(void *arg)
{
    char *plate = (char *)arg;
    // Calculate cost
    // No need to use hash lock here as just reading the value which won't be changed by anything else at this point
    car_t *car = htab_find(&verified_cars, plate);
    float duration = (float)duration_ms(car->entry_time);
    float cost = duration * 0.05;

    // Increment total revenue
    pthread_mutex_lock(&revenue_lock);
    total_revenue += cost;
    pthread_mutex_unlock(&revenue_lock);

    // Update billing records
    pthread_mutex_lock(&billing_lock);
    FILE* bill = fopen("billing.txt", "a+");
    fprintf(bill, "%s $.2f", plate, cost);
    fclose(bill);
    pthread_mutex_unlock(&billing_lock);
}

void generate_GUI( carpark_t *data )
{
    while (true)
    {
        usleep(50);
        printf("\033[2J"); // Clear screen

        printf(
        "╔═══════════════════════════════════════════════╗\n"
        "║                CARPARK SIMULATOR              ║    by DAVID AND DANIEL \n"
        "╚═══════════════════════════════════════════════╝\n"
        );
        
        printf("\n\e[1mTotal Revenue:\e[m $0.00\n\n");


        printf("\n\e[1mLevel\tCapacity\tLPR\t\tTemp (C)\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 1; i <= NUM_LEVELS; i++)
        {
            printf("%d\t", i);
            printf("%d/%d\t\t", i, CARS_PER_LEVEL);
            printf("%s\t\t", "123ABC");
            printf("%d\t\t", 27);
            printf("\n");
        }

        printf("\n");

        printf("\n\e[1mEntry\tGate\t\tLPR\t\tDisplay\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 1; i <= NUM_ENTRIES; i++)
        {
            printf("%d\t", i);
            printf("%s\t\t", gate_status('C'));
            printf("%s\t\t", "123ABC");
            printf("%c\t", '3');
            printf("\n");
        }

        printf("\n");
        
        printf("\n\e[1mExit\tGate\t\tLPR\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 1; i <= NUM_ENTRIES; i++)
        {
            printf("%d\t", i);
            printf("%s\t", gate_status('L'));
            printf("%s\t", "123ABC");
            printf("\n");
        }

        printf("\n");
    }
}

char *gate_status(char code)
{
    switch((int)code)
    {
        case 67:
            return "Closed";
            break;
        case 79:
            return "Open";
            break;
        case 76:
            return "Lowering";
            break;
        case 82:
            return "Raising";
            break;
        default:
            return NULL;
            break;
    }

}

long long duration_ms(struct timeval start) {

    struct timeval end;
    gettimeofday(&end,NULL);

    return (((long long)end.tv_sec)*1000)+(end.tv_usec/1000) - (((long long)start.tv_sec)*1000)+(start.tv_usec/1000);
}