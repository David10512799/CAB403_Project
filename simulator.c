#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>

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
char create_car();

bool init_carpark(shared_carpark_t* carpark);

void init_carpark_values(carpark_t* park);




int main(int argc, char **argv){

    shared_carpark_t carpark;
    
    if (!init_carpark(&carpark))
    {
        fprintf(stderr, "Error initialising shared memory: %s\n", strerror(errno));
    }




    htab_t verified_cars;
    int buckets = 40;
        
    if (!htab_init(&verified_cars, buckets))
    {
        printf("failed to initialise hash table\n");
        return EXIT_FAILURE;
    }

    htab_insert_plates(&verified_cars);



    // for(;;)
    // {   
    //     ms_pause( 100 ); // random number between 1 to 100;
    //     car_t *car; 
    //     char *plate;

    //     do {
    //         plate = generate_plate();
    //         car = htab_find(&verified_cars, plate);
    //         if ( car == NULL ) break;
    //     } while ( car->in_carpark ); 


    //     pthread_t car;
    //     pthread_create(&car, NULL, create_car, plate);

    // }

    return 0;
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
