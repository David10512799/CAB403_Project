#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "carpark.h"

//Car Simulation

//Declare function that builds the LPR (License plate reader)
void LPR();

//Declare Function to create a simulated car with random license plate
//Simulated car queues up at a random entrance triggering LPR when queue position = 0/1.
//Parameters: plates.txt --> plateList
char createCar();

//A function to check if the license plate of the car at the front of the queue is in plates.txt
void searchPlates();

//Declare function that runs after LPR is triggered to display digital sign.
//Parameters
char digitalSign(char licensePlate);

//The car will move to the chosen level by the manager and displayed on the digital sign.
void moveToLevel(int level, char licensePlate);

//The car will park for a random period of time.
void timeParked(char licensePlate);

//After the car is finished parking, the LPR will be triggered and the car will move to an exit and trigger
//the exit LPR and the boom gate will open. The car will exit the simulation.
//void exit(char licensePlate);

//BOOM GATE
void boomGateWait();

//TEMPERATURE SENSOR
void updateTemp();

//Create a license plate that is either unique or matches with a license plate in the plates.txt file.
char createCar();



int main(int argc, char **argv){

    shared_carpark_t carpark;
    
    if (!init_carpark(&carpark))
    {
        fprintf(stderr, "Error initialising shared memory: %s\n", strerror(errno));
    }

    // char **plates;
    // int platecount = 0;
    // char plate[7];
    // while(fscanf("plates.txt", "%s", plate) != EOF)
    // {
    //     plates[platecount] = malloc();
    //     platecount++;
    // }

    return 0;
}








