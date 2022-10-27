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


size_t buckets = 40;
htab_t verified_cars;

pthread_mutex_t space_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t revenue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hash_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t billing_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t space_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t revenue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t hash_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t billing_cond = PTHREAD_COND_INITIALIZER;



volatile int freespaces[LEVELS];
volatile float total_revenue = 0.00;

typedef struct level_LPR_monitor level_LPR_monitor_t;
struct level_LPR_monitor
{
    int id;
    LPR_t *level_LPR;
};



//Function declarations
void *generate_GUI(void *arg); 
char *gate_status(char code); void open_gate(); void close_gate(); 
void *generate_bill(void *arg);
void generate_car(char *plate, int level);
char find_space();
long long duration_ms(struct timeval start);
void *delete_car(void *arg);
bool string_equal(char *a, char *b);

// Monitors
void *monitor_entry(void *arg); void *monitor_exit(void *arg); void *monitor_level(void *arg);
void *monitor_gate(void *arg);

int main(void) {


    // Initialise hashtable and insert plates from "plates.txt"

    if (!htab_init(&verified_cars, buckets))
    {
        printf("1\n");
        printf("failed to initialise hash table\n");
        return EXIT_FAILURE;
    }

    htab_insert_plates(&verified_cars);


    // Get shared memory object
    shared_carpark_t carpark;
    carpark.name = SHARE_NAME;
    if ( (carpark.fd = shm_open(SHARE_NAME, O_RDWR, 0)) < 0 ){
        perror("shm_open");
        carpark.data = NULL;
        return false;
    }
    if ((carpark.data = mmap(0, sizeof(carpark_t),PROT_READ | PROT_WRITE, MAP_SHARED, carpark.fd, 0)) == (void *)-1 ) {
        perror("mmap");
        return false;
    }
    // Initialise the list of cars with all spaces remaining
    for( int i = 0; i < LEVELS; i++){
        freespaces[i] = CARS_PER_LEVEL;
    }

    // Generate GUI
    pthread_t gui;
    pthread_create(&gui, NULL, generate_GUI, &carpark);

    // Begin monitoring Entrance License Plate Readers to detect Cars and Boom Gates put into OPEN status
    for( int i = 0; i < ENTRIES; i++){
        pthread_t entry_LPR, entry_gate;
        pthread_create(&entry_LPR, NULL, monitor_entry, &carpark.data->entrance[i]); //needs access to LPR and gate
        pthread_create(&entry_gate, NULL, monitor_gate, &carpark.data->entrance[i].gate);
    }

    // Begin monitoring Level License Plate Readers to detect Cars
    level_LPR_monitor_t monitors[LEVELS];

    for( int i = 0; i < LEVELS; i++){
        // initialise struct for passing to thread so it knows which level to change the car to
        monitors[i].level_LPR = &carpark.data->level[i].LPR;
        monitors[i].id = i;

        pthread_t level_LPR;
        pthread_create(&level_LPR, NULL, monitor_level, &monitors[i]);
    }
    
    // Begin monitoring Exit License Plate Readers to detect Cars
    for ( int i = 0; i < EXITS; i++)
    {
        pthread_t exit_LPR, exit_gate;
        pthread_create(&exit_LPR, NULL, monitor_exit, &carpark.data->exit[i]); // needs access to LPR and gate
        pthread_create(&exit_gate, NULL, monitor_gate, &carpark.data->exit[i].gate);
    }
    

    while (true)
    {
        sleep(10);
    }

    return EXIT_SUCCESS;

}


void *monitor_gate(void *arg)
{
    pthread_setschedprio(pthread_self(), 19);
    

    gate_t *gate = (gate_t *)arg;

    for(;;)
    {
        //lock gate mutex and wait for signal
        pthread_mutex_lock(&gate->mutex);
        while(gate->status != OPEN)
            pthread_cond_wait(&gate->condition, &gate->mutex);

        // delay for 20ms * TIMEX
        pthread_mutex_unlock(&gate->mutex);
        ms_pause(20);
        pthread_mutex_lock(&gate->mutex);
        // Set the gate status to Lowering
        gate->status = LOWERING;
    
        // signal condition variable and unlock mutex
        pthread_cond_broadcast(&gate->condition);
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
        while (string_equal(lpr->level_LPR->plate, EMPTY_LPR))
            pthread_cond_wait(&lpr->level_LPR->condition, &lpr->level_LPR->mutex);
        int index = lpr->id;
        // printf("Index %d %d\n", index, lpr.id);
        // Corrected from indexing value to actual level
        int level = index + 1; 
        char *plate = lpr->level_LPR->plate;

        // get pointer to car
        car_t *car = htab_find(&verified_cars, plate);

        // check car is supposed to be on level and that there is room on that level and update values if not
        // if car is supposed to be, no need to change these values
        // if car is not supposed to be, and there is also no room, no need to change these values
        // if car is not supposed to be, and there is room, values must be changed
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

        // reset LPR value and unlock mutex
        ms_pause(10);
        strcpy(lpr->level_LPR->plate, EMPTY_LPR);
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
        while (string_equal(exit->LPR.plate, EMPTY_LPR))
            pthread_cond_wait(&exit->LPR.condition, &exit->LPR.mutex);

        // Read plate
        char *plate = exit->LPR.plate;

        // Bill car
        pthread_t bill;
        pthread_create(&bill, NULL, generate_bill, plate);
        pthread_join(bill, NULL);

        // Remove car from carpark
        car_t * car = htab_find(&verified_cars, plate);
        int index = car->current_level - 1;
        freespaces[index]++;

        remove_car(&verified_cars, plate);

        // Lock gate mutex
        pthread_mutex_lock(&exit->gate.mutex);
        // Set gate status to RAISING
        exit->gate.status = RAISING;
        // Signal condition signal and unlock mutex
        pthread_cond_broadcast(&exit->gate.condition);
        pthread_mutex_unlock(&exit->gate.mutex);

        // unlock lpr mutex and reset LPR
        ms_pause(10);
        strcpy(exit->LPR.plate, EMPTY_LPR);
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
        while (string_equal(entry->LPR.plate, EMPTY_LPR))
            pthread_cond_wait(&entry->LPR.condition, &entry->LPR.mutex);
        
        // Read plate and determine if allowed in.
        char *plate = entry->LPR.plate;
        char space;

        // Acquire hash and space locks out here so that car can be generated without losing its spot
        // pthread_mutex_lock(&hash_lock);
        // pthread_mutex_lock(&space_lock);

        if ( htab_search_plate(&verified_cars, plate) )
        {
            space = find_space();
        }
        else
        {
            space = DENIED;
        }

        // Display entry status on sign
        pthread_mutex_lock(&entry->sign.mutex);

        entry->sign.display = space;

        pthread_cond_signal(&entry->sign.condition);
        pthread_mutex_unlock(&entry->sign.mutex);
        // Set gate to raising if level = 1 - 5
        int level = (int)space - 48;
        if ((0 < level) && (level < 6))
        {
            freespaces[level - 1]--;

            generate_car(plate, level);

            pthread_mutex_lock(&entry->gate.mutex);
            while( entry->gate.status != CLOSED)
            {
                pthread_cond_wait(&entry->gate.condition, &entry->gate.mutex);
            }
            entry->gate.status = RAISING;

            pthread_cond_broadcast(&entry->gate.condition);
            pthread_mutex_unlock(&entry->gate.mutex);
        }

        // pthread_mutex_unlock(&hash_lock);
        // pthread_mutex_unlock(&space_lock);


        // Reset entry status on sign
        ms_pause(10);
        pthread_mutex_lock(&entry->sign.mutex);
        entry->sign.display = EMPTY_SIGN;
        pthread_mutex_unlock(&entry->sign.mutex);

        // reset lpr unlock lpr mutex
        strcpy(entry->LPR.plate, EMPTY_LPR);
        pthread_mutex_unlock(&entry->LPR.mutex);
    }

}

// Find the level with the most spots remaining or return FULL
char find_space()
{
    char retVal = FULL;
    int level = 1;
    int highest = freespaces[0];
    for( int i = 1; i < LEVELS; i++){
        if(freespaces[i] > highest)
        {
            level = i + 1;
            highest = freespaces[i];
        }
        // printf("freespaces %d\n", freespaces[i-1]);
    }
    // printf("freespaces %d\n", freespaces[4]);
    // printf("level %d\n", level);

    if (level != 0)
    {
        retVal = level + '0';
    }
    // printf("space %c\n", retVal);


    return retVal;
}

void generate_car(char *plate, int level)
{
    car_t new_car;
    sprintf(new_car.plate, plate);
    new_car.current_level = level;

    struct timeval entry_time;
    gettimeofday(&entry_time, NULL);

    new_car.entry_time = entry_time;

    add_car(&verified_cars, new_car);
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
    fprintf(bill, "%s $%.2f\n", plate, cost);
    fclose(bill);
    pthread_mutex_unlock(&billing_lock);

    return NULL;
}

void *generate_GUI( void *arg )
{
    shared_carpark_t *carpark = (shared_carpark_t *)arg;
    carpark_t *data = carpark->data;
    
    for(;;)
    {
        

        printf(
        "╔═══════════════════════════════════════════════╗\n"
        "║                CARPARK SIMULATOR              ║\n"
        "╚═══════════════════════════════════════════════╝\n"
        );
        //Total revenue of 
        printf("\n\e[1mTotal Revenue:\e[m $%.2f\n\n", total_revenue);

        // Level information
        printf("\n\e[1mLevel\tCapacity\tLPR\t\tTemp (C)\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 0; i < LEVELS; i++)
        {
            printf("%d\t", i + 1); // level no. corrected from indexing value
            printf("%d/%d\t\t", CARS_PER_LEVEL - freespaces[i], CARS_PER_LEVEL); // capacity X out of Y
            printf("%s\t\t", data->level[i].LPR.plate); // Level LPR reading
            printf("%d\t\t", data->level[i].temperature.sensor);  // Level temperature reading
            printf("\n");
        }
        fflush(stdout);
        printf("\n");
        // Entry information
        printf("\n\e[1mEntry\tGate\t\tLPR\t\tDisplay\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 0; i < ENTRIES; i++)
        {
            printf("%d\t", i + 1); // entry no. corrected from indexing value
            printf("%s\t", gate_status(data->entrance[i].gate.status)); // state of entry gate
            printf("%s\t\t", data->entrance[i].LPR.plate); // Entry LPR reading
            printf("%c\t", data->entrance[i].sign.display); // Entry information sign display value
            printf("\n");
        }
        fflush(stdout);
        printf("\n");
        // Exit information
        printf("\n\e[1mExit\tGate\t\tLPR\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 0; i < ENTRIES; i++)
        {
            printf("%d\t", i + 1); // exit no. corrected from indexing value
            printf("%s\t", gate_status(data->exit[i].gate.status)); // state of exit gate
            printf("%s\t", data->exit[i].LPR.plate); // Exit LPR reading
            printf("\n");
        }
        fflush(stdout);
        printf("\n");
        ms_pause(50);
        printf("\033[2J"); // Clear screen
    }
    return NULL;
}
// Returns the meaning of the gate status characters stored in shared memory
char *gate_status(char code)
{
    switch((int)code)
    {
        case 67:
            return "Closed  ";
            break;
        case 79:
            return "Open    ";
            break;
        case 76:
            return "Lowering";
            break;
        case 82:
            return "Raising ";
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

