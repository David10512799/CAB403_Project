#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "carpark.h"
#include "carlist.h"

extern int errno;

//Car Simulation

//Declare function that builds the LPR (License plate reader)
void LPR();

//Declare Function to create a simulated car with random license plate
//Simulated car queues up at a random entrance triggering LPR when queue position = 0/1.
//Parameters: plates.txt --> plateList
char generate_plate();

//A function to check if the license plate of the car at the front of the queue is in plates.txt
void search_plates();

//Declare function that runs after LPR is triggered to display digital sign.
//Parameters
char digital_sign(char licensePlate);

//The car will move to the chosen level by the manager and displayed on the digital sign.
void move_to_Level(int level, char licensePlate);

//The car will park for a random period of time.
void time_parked(char licensePlate);

//After the car is finished parking, the LPR will be triggered and the car will move to an exit and trigger
//the exit LPR and the boom gate will open. The car will exit the simulation.
void exit_carpark(char licensePlate);

//BOOM GATE
void boom_gate_wait();

//TEMPERATURE SENSOR
void update_temp();

//Create a license plate that is either unique or matches with a license plate in the plates.txt file.
void generate_car();
char create_car();

bool init_carpark(shared_carpark_t* carpark);

void init_carpark_values(carpark_t* park);


void *sim(void *arg);

void pause_for(int time);

//Global Variables
char plate_registry[100][7];
char plate[PLATE_LENGTH];
int rand_plate;



int main(void){

    shared_carpark_t carpark;
    
    if (!init_carpark(&carpark))
    {
        fprintf(stderr, "Error initialising shared memory: %s\n", strerror(errno));
    }

    pthread_t simulation;
    pthread_create(&simulation, NULL, sim, &carpark);

    while(true){
        sleep(2);
    }


    return EXIT_SUCCESS;
}

// create thread
void *sim(void *arg)
{  
    carpark_t *data = (carpark_t *)arg;
    htab_t verified_cars;

    int buckets = 40;
    char plate;

    if (!htab_init(&verified_cars, buckets))
    {
        printf("failed to initialise hash table\n");
    }
    htab_insert_plates(&verified_cars);
    
    for(;;)
        {
            ms_pause(1); //random number between 1 to 100;
            car_t *car;
            char *plate;
            
            do{
            //generate_plate();
            srand(time(NULL));
            rand_plate = rand() % 100;
            

            car = htab_find(&verified_cars, plate);

            printf("%s\n", car->plate);

            if(car == NULL){
                break;
            }
            }while(car->in_carpark);


        }
 }

char generate_plate(){
    FILE* input_file = fopen("plates.txt", "r");

    if (input_file == NULL)
    {
        fprintf(stderr, "Error: failed to open file %s", "plates.txt");
        exit(1);
    }

    int i = 0;
    while( fscanf(input_file, "%s", plate) != EOF )
    {
        sprintf(plate_registry[i], plate);
        //printf("%s\n", plate_registry[i]);
        i++;
    }

    fclose(input_file);

    return *plate_registry[rand_plate];
}

void generate_car(){

}

bool init_carpark(shared_carpark_t* carpark) 
{

    shm_unlink(SHARE_NAME);

    carpark->name = SHARE_NAME;

    if ((carpark->fd = shm_open(SHARE_NAME, O_CREAT | O_RDWR , 0666)) < 0 ){
        perror("shm_open");
        printf("Failed to create shared memory object for Carpark\n");
        exit(1);
    }

    if (ftruncate(carpark->fd, sizeof(carpark_t))){
        perror("ftruncate");
        printf("Failed to initialise size of shared memory object for Carpark\n");
        exit(1);
    }

    if  ((carpark->data = mmap(0, sizeof(carpark_t), PROT_READ | PROT_WRITE, MAP_SHARED, carpark->fd, 0)) == (void *)-1 ){
        perror("mmap");
        printf("Failed to map the shared memory for Carpark");
        exit(1);
    }

    init_carpark_values(carpark->data);

    return true;
}



void init_carpark_values(carpark_t* park)
{

    pthread_mutexattr_t mutex_attr;
    if (pthread_mutexattr_init(&mutex_attr) != 0)
    {
        perror("pthread_mutexattr_init");
        exit(1);
    }
    if ( pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        perror("pthread_mutexattr_setpshared");
        exit(1);
    }

    pthread_condattr_t cond_attr;
    if (pthread_condattr_init(&cond_attr) != 0)
    {
        perror("pthread_mutexattr_init");
        exit(1);
    }
    if ( pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        perror("pthread_mutexattr_setpshared");
        exit(1);
    }    

    for (int i = 0; i < 5; i++)
    {
        // ENTRANCES
        park->entrance[i].gate.status = 'C';
        pthread_mutex_init(&park->entrance[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].gate.condition, &cond_attr);

        
        park->entrance[i].LPR.plate = EMPTY_LPR;
        pthread_mutex_init(&park->entrance[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].LPR.condition, &cond_attr);

        pthread_mutex_init(&park->entrance[i].sign.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].sign.condition, &cond_attr);        

        // EXITS
        park->exit[i].gate.status = 'C';
        pthread_mutex_init(&park->exit[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].gate.condition, &cond_attr);

        park->exit[i].LPR.plate = EMPTY_LPR;
        pthread_mutex_init(&park->exit[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].LPR.condition, &cond_attr);

        // LEVELS
        park->level[i].LPR.plate = EMPTY_LPR;
        pthread_mutex_init(&park->level[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->level[i].LPR.condition, &cond_attr);

        park->level[i].temperature.alarm = 0;
        park->level[i].temperature.sensor = 0;
        
    }
}
