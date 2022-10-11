#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
 
#include "carpark.h"
#include "plates.h"

size_t buckets = 40;
htab_t verified_cars;


typedef struct car car_t;
struct car
{
    char *plate;
    int current_level;
    struct timeval entry_time;
};

typedef struct car_list car_list_t;
struct car_list
{
    int free_spaces;
    car_t cars[CARS_PER_LEVEL];
};

car_list_t list_cars[NUM_LEVELS];
pthread_mutex_t list_lock;
pthread_cond_t full, empty;

//Function declarations
void generate_GUI(carpark_t *data); 
char *gate_status(char code); void open_gate(); void close_gate(); 
void generate_bill(char *plate, int useconds);
void generate_car(char *plate, int level);
char find_space();
long long duration_ms(struct timeval start);

// License plate Readers Monitor
void *monitor_entry(void *arg); void *monitor_exit(void *arg); void *monitor_level(void *arg);




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
        list_cars[i].free_spaces = CARS_PER_LEVEL;
    }

    // Begin monitoring Entrance License Plate Readers to detect Cars
    for( int i = 0; i < NUM_ENTRIES; i++){
        pthread_t entry_LPR;
        pthread_create(&entry_LPR, NULL, monitor_entry, &carpark.data->entrance[i]);
    }

    // Begin monitoring Level License Plate Readers to detect Cars
    for( int i = 0; i < NUM_LEVELS; i++){
        pthread_t level_LPR;
        pthread_create(&level_LPR, NULL, monitor_level, &carpark.data->level[i].LPR);
    }
    
    // Begin monitoring Exit License Plate Readers to detect Cars
    for ( int i = 0; i < NUM_EXITS; i++)
    {
        pthread_t exit_LPR;
        pthread_create(&exit_LPR, NULL, monitor_exit, &carpark.data->exit[i].LPR);
    }

    while (true) sleep(10);

    return EXIT_SUCCESS;

}

void *monitor_level(void *arg)
{
    LPR_t *lpr = (LPR_t *)arg;

    for(;;)
    {
        // lock lpr mutex and wait for signal
        pthread_mutex_lock(&lpr->mutex);
        while (lpr->plate == EMPTY_LPR)
            pthread_cond_wait(&lpr->condition, &lpr->mutex);

        // Read plate

        // unlock mutex
        pthread_mutex_unlock(&lpr->mutex);
    }
}

void *monitor_exit(void *arg)
{
    LPR_t *lpr = (LPR_t *)arg;

    for(;;)
    {
        // lock lpr mutex and wait for signal
        pthread_mutex_lock(&lpr->mutex);
        while (lpr->plate == EMPTY_LPR)
            pthread_cond_wait(&lpr->condition, &lpr->mutex);

        // Read plate


        // unlock lpr mutex 
        pthread_mutex_unlock(&lpr->mutex);
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

        if ( htab_search_value(&verified_cars, plate) )
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
        level = list_cars[i].free_spaces > list_cars[i - 1].free_spaces ? i + 1 : i;
    }

    if (level != 0)
        retVal = (char)level;

    return retVal;
}

void generate_car(char *plate, int level)
{
    car_t car;
    car.current_level = level;
    car.plate = plate;
    gettimeofday(&car.entry_time, NULL);

    // add to hashtable

}

void generate_bill(char *plate, int useconds)
{
    float cost = useconds * 0.05;

    FILE* bill = fopen("billing.txt", "a+");

    fprintf(bill, "%s $.2f", plate, cost);

    fclose(bill);
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

long long timeInMilliseconds(struct timeval start) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}