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

#include "carpark.h"
#include "carlist.h"

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

    return EXIT_SUCCESS;
}

// create thread
void *sim(void *arg)
{  
    carpark_t *data = (carpark_t *)arg;

    htab_t *verified_cars;
    int buckets = 40;
    char plate;

    if (!htab_init(verified_cars, buckets))
    {
        printf("failed to initialise hash table\n");
    }
    
    htab_insert_plates(verified_cars);
    
    for(;;)
        {
            pause_for(1); //random number between 1 to 100;
            car_t *car;
            char *plate;
            
            do{
            generate_plate();
            srand(time(NULL));
            rand_plate = rand() % 100;
            printf("%s\n", plate_registry[rand_plate]);

            car = htab_find(verified_cars, plate);

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
        strcpy(plate_registry[i], plate);
        //printf("%s\n", plate_registry[i]);
        i++;
    }

    fclose(input_file);

    
    return *plate_registry[rand_plate];
}

void generate_car(){

}



#define TIMEX 1 // Time multiplier for timings to slow down simulation - set to 1 for specified timing

void pause_for(int time)
{
    usleep(TIMEX * time);
}


