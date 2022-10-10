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

shared_carpark_t carpark;


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

#define NUM_BUCKETS 20


typedef struct item item_t;
struct item
{
    char *value;
    void *next;
};


typedef struct htab htab_t;
struct htab
{
    item_t *buckets;
    size_t size;
};

htab_t h;

bool htab_add(size_t key, char *value);

bool htab_init();

size_t htab_index(htab_t *h, size_t key);

item_t *htab_bucket(htab_t *h, int key);

size_t djb_hash(char *c);

bool htab_insert_plates();

void init();



int main(int argc, char **argv){

    // init();
    init_carpark(&carpark);


    return 0;
}




// bool htab_insert_plates()
//     {
//     FILE* input_file = fopen("plates.txt", "r");
//     int num;
//     int number_plates;

//     if (input_file == NULL)
//     {
//         fprintf(stderr, "Error: failed to open file %s", "plates.txt");
//         exit(1);
//     }

//     char plate[6];
//     while(fscanf(input_file, "%s", plate) != EOF)
//     {
//         size_t key = djb_hash(plate) % h.size;
//         htab_add(key, plate);
//     }

//     fclose(input_file);
//     return true;
//     }






