#ifndef HEADERS
    #define HEADERS
    #include <sys/mman.h> 
    #include <sys/stat.h> 
    #include <fcntl.h> 
    #include <pthread.h> 
    #include <stdbool.h> 
    #include <unistd.h>
#endif

#define SHARE_NAME "PARKING"

typedef struct gate gate_t;
struct gate
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char status;
};

typedef struct LPR LPR_t;
struct LPR
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char plate[6];
};

typedef struct sign sign_t;
struct sign
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char display;
};

typedef struct fire_alarm fire_alarm_t;
struct fire_alarm
{
    short temperature;
    int alarm;
};

typedef struct entrance entrance_t;
struct entrance
{
    LPR_t LPR;
    gate_t gate;
    sign_t sign;
};

typedef struct exit exit_t;
struct exit
{
    LPR_t LPR;
    gate_t gate;
};

typedef struct level level_t;
struct level
{
    LPR_t LPR;
    fire_alarm_t fire_alarm;
};

typedef struct carpark carpark_t;
struct carpark
{
    entrance_t entrance1;
    entrance_t entrance2;
    entrance_t entrance3;
    entrance_t entrance4;
    entrance_t entrance5;
    exit_t exit1;
    exit_t exit2;
    exit_t exit3;
    exit_t exit4;
    exit_t exit5;
    level_t level1;
    level_t level2;
    level_t level3;
    level_t level4;
    level_t level5;
};

typedef struct shared_carpark shared_carpark_t;
struct shared_carpark
{
    const char* name;
    int fd;
    carpark_t* data;
};

void init_carpark(shared_carpark_t* carpark) 
{

    shm_unlink(SHARE_NAME);

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

}

void get_carpark(shared_carpark_t* carpark)
{
    if ((carpark->fd = shm_open(SHARE_NAME, O_RDWR, 0666)) < 0)
    {
        perror("shm_open");
        printf("Carpark data does not exist\n");
        exit(1);
    }
}