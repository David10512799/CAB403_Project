#include "simulator.h"


shared_carpark_t carpark;
htab_t verified_cars;

pthread_mutex_t entry_mutex[ENTRIES];
pthread_cond_t entry_cond[ENTRIES];
pthread_mutex_t exit_mutex[EXITS];
pthread_cond_t exit_cond[EXITS];
pthread_mutex_t valid_lock = PTHREAD_MUTEX_INITIALIZER;

int plate_count;

int run_mode;

int end_monitors = 0;

int valid = 1;

int main(int argc, char **argv){

    if (argc == 1)
    {
        run_mode = 0;
    }
    else
    {
        run_mode = (int)argv[1][0] - 48;
    }

    // Generate Shared Memory    
    if (!init_carpark(&carpark))
    {
        fprintf(stderr, "Error initialising shared memory: %s\n", strerror(errno));
    }

    // Generate Hash Table

    int buckets = 40;

    if (!htab_init(&verified_cars, buckets))
    {
        printf("failed to initialise hash table\n");
    }
    
    htab_insert_plates(&verified_cars);

    // Initialise mutex and cond
    for (int i = 0; i < ENTRIES; i++)
    {
       pthread_mutex_init(&entry_mutex[i], NULL);
       pthread_cond_init(&entry_cond[i], NULL);
    }
    for (int i = 0; i < EXITS; i++)
    {
       pthread_mutex_init(&exit_mutex[i], NULL);
       pthread_cond_init(&exit_cond[i], NULL);
    }
    
    
    plate_count = get_plate_count();

    // Create array and store plate char[] to it
    char plate_registry_temp[plate_count][PLATE_LENGTH];
    FILE* input_file = fopen("plates.txt", "r");
    for (int i = 0; fscanf(input_file, "%s", plate_registry_temp[i]) != EOF; i++)
    {
    }
    fclose(input_file);

    // Create pointer array to each of the plates
    // char **plate_registry = calloc(plate_count, PLATE_LENGTH);
    char *plate_registry[plate_count];
    for (int i = 0; i < plate_count; i++)
    {
        plate_registry[i] = plate_registry_temp[i];
    }

    // monitor entry boom gates transitions
    for( int i = 0; i < ENTRIES; i++){
        pthread_t gate;
        pthread_create(&gate, NULL, monitor_gate, &carpark.data->entrance[i].gate);
    }

    // monitor exit boom gates transitions
    for( int i = 0; i < EXITS; i++){
        pthread_t gate;
        pthread_create(&gate, NULL, monitor_gate, &carpark.data->exit[i].gate);
    }

    // Generate level temperatures
    for (int i = 0; i < LEVELS; i++)
    {
        pthread_t level_temp;
        pthread_create(&level_temp, NULL, temp_sim, &carpark.data->level[i].temperature);
    }
    

    // Generate cars until fire
    start_car_simulation(plate_registry);

    // Wait for all cars to leave carpark and manager turns off alarms
    while(carpark.data->level[0].temperature.alarm == 1)
    {       
        ms_pause(10);
    }

    end_monitors = 1;

    // Unmap and unlink shared memory and destory hash table
    munmap(&carpark.data, sizeof(carpark_t));
    shm_unlink(SHARE_NAME);
    carpark.data = NULL;
    carpark.fd = -1;
    htab_destroy(&verified_cars);

    // Destory all local mutexes and conditions
    for (int i = 0; i < ENTRIES; i++)
    {
        pthread_mutex_destroy(&entry_mutex[i]);
        pthread_cond_destroy(&entry_cond[i]);
    }
    for (int i = 0; i < EXITS; i++)
    {
        pthread_mutex_destroy(&exit_mutex[i]);
        pthread_cond_destroy(&exit_cond[i]);
    }
    pthread_mutex_destroy(&valid_lock);
    

    return EXIT_SUCCESS;
}
int normal_temp(int current_temp){
    int direction = rand() % 2;

    if (direction)
    {
        if (current_temp < 30)
        {
            current_temp++;
        } 
        else
        {
            current_temp--;
        }
    }
    else
    {
        if (current_temp > 15)
        {
            current_temp--;                
        } 
        else
        {
            current_temp++;
        }
    }
    return current_temp;    
    
}
void *temp_sim(void *arg){
    temperature_t *sensor = (temperature_t *)arg;
    int temp = 22;
    time_t start_time = time(NULL);
    while (!sensor->alarm)
    {

        // Randomly raise or decrease temp by 1 with a max of 30 and min of 15
        switch (run_mode)
        {
        case 0: // Normal temp generation
            temp = normal_temp(temp);
            break;
        case 1: // Trigger alarm by rate of raise fire
            if ((start_time - time(NULL)) < -20) // After 20 seconds since starting
            {
                temp += 15;
            }   
            else
            {
                temp = normal_temp(temp);
            }
            break;
        case 2: // Trigger alarm by fixed temperature fire
            if ((start_time - time(NULL)) < -20)
            {
                temp += 1;
            }
            else
            {
                temp = normal_temp(temp);
            }
            break;
        case 3: // Trigger alarm by hardware failure
            if ((start_time - time(NULL)) < -20)
            {
                ;
            }
            else
            {
                temp = normal_temp(temp);
            }
            break;
        default:
            break;
        }
        
        sensor->sensor = temp;
        // int rand_pause = (rand() % 5) + 1;
        ms_pause(100);
    }
    return NULL;
}

void *monitor_gate(void *arg)
{
    gate_t *gate = (gate_t *)arg;

    while(!end_monitors)
    {
        //lock gate mutex and wait for signal
        pthread_mutex_lock(&gate->mutex);
        while((gate->status == CLOSED) || (gate->status == OPEN))
        {
            pthread_cond_wait(&gate->condition, &gate->mutex);
        }
        // delay for 10ms * TIMEX
        printf("gate status is %c\n", gate->status);
        ms_pause(10);
        
        char status = gate->status;

        if (status == LOWERING)
        {
            gate->status = CLOSED;
        }
        if (status == RAISING)
        {
            gate->status = OPEN;
        }
        printf("gate status is %c\n", gate->status);

        // signal condition variable and unlock mutex
        pthread_cond_broadcast(&gate->condition); // Signal manager it set to open or closed
        // pthread_cond_broadcast(&localGate); // Signal car to check boomgate status
        pthread_mutex_unlock(&gate->mutex);
    }

    return NULL;
}

void start_car_simulation(char **plate_registry){
    // Simulate cars while firealarms are off
    while (!carpark.data->level[0].temperature.alarm) 
    {   

        int rand_wait = (rand() % 99) + 1;
        srand(time(NULL));
      
        pthread_t car_sim;
        pthread_create(&car_sim, NULL, sim_car, plate_registry);

        ms_pause(rand_wait); // Random time between 1 and 100             

    }    
}

// create thread
void *sim_car(void *arg)
{  
    pthread_setschedprio(pthread_self(), -20);
    char **plate_registry = (char**)arg;
    char plate[PLATE_LENGTH];
    char *key;
    char rand_plate[PLATE_LENGTH];
    car_t *car;
    node_t *in_line;
    int gen_pause = 0;
    srand(time(NULL));

    if (valid)
    {
        do {                
            int plate_number;
            plate_number = (rand() % (plate_count));
            key = plate_registry[plate_number]; // random number between 0 and plate_count
            car = htab_find(&verified_cars, key);
            // lock all entry list mutexes
            for (int i = 0; i < ENTRIES; i++)
            {
                pthread_mutex_lock(&entry_mutex[i]);
            }
            in_line = node_find_name_array(entry_list, key, ENTRIES);
            for (int i = 0; i < ENTRIES; i++)
            {
                pthread_mutex_unlock(&entry_mutex[i]);
            }          
            // printf("valid dupe\n");
            gen_pause++;
            if (gen_pause > 100)
            {
                ms_pause(10);
                gen_pause -= 10;
            }
            
        } while(car->in_carpark || in_line != NULL);

        strcpy(plate, key);

        pthread_mutex_lock(&valid_lock);
        valid = 0;
        pthread_mutex_unlock(&valid_lock);
    } 
    else
    {
        do
        {            
            for (int i = 0; i < 3; i++)
            {
                rand_plate[i] = (rand() % 10) + 48;
            }
            for (int i = 3; i < 6; i++)
            {
                rand_plate[i] = (rand() % 26) + 65;
            }
            rand_plate[6] = '\0';
            car = htab_find(&verified_cars, rand_plate);
            for (int i = 0; i < ENTRIES; i++)
            {
                pthread_mutex_lock(&entry_mutex[i]);
            }
            in_line = node_find_name_array(entry_list, rand_plate, ENTRIES);
            for (int i = 0; i < ENTRIES; i++)
            {
                pthread_mutex_unlock(&entry_mutex[i]);
            } 
        } while ( car != NULL || in_line != NULL);
        pthread_mutex_lock(&valid_lock);
        valid = 1;
        pthread_mutex_unlock(&valid_lock);
        strcpy(plate, rand_plate);
    }
    printf("plate is %.6s\n", plate);
   
    

    int random_entry = rand() % ENTRIES;
    int random_exit = rand() % EXITS;
    
    // Add car to queue
    pthread_mutex_lock(&entry_mutex[random_entry]);
    // node_t *new_head = node_add(entry_list[random_entry], plate);
    entry_list[random_entry] = node_add(entry_list[random_entry], plate);;   
    printf("%.6s joined the line for entry %d\n", entry_list[random_entry]->plate, random_entry + 1);
    pthread_mutex_unlock(&entry_mutex[random_entry]); 

    // Wait in line until boomgate is available    
    pthread_mutex_lock(&entry_mutex[random_entry]);
    while(node_find_name(entry_list[random_entry], plate)->next != NULL){
        pthread_cond_wait(&entry_cond[random_entry], &entry_mutex[random_entry]);

    } 
    pthread_mutex_unlock(&entry_mutex[random_entry]);

    // Wait at boomgate until it is fully closed
    printf("%.6s arrived at boomgate %d\n", plate, random_entry + 1);
    // pthread_mutex_lock(&carpark.data->entrance[random_entry].gate.mutex);
    // while(carpark.data->entrance[random_entry].gate.status != CLOSED)
    // {
    //     printf("%.6s waiting on gate to close. gate is %c\n", plate, carpark.data->entrance[random_entry].gate.status);
    //     pthread_cond_wait(&carpark.data->entrance[random_entry].gate.condition, &carpark.data->entrance[random_entry].gate.mutex);
    //     // printf("%.6s gate signaled. gate is %c\n", plate, carpark.data->entrance[random_entry].gate.status);
    // }
    // // printf("%.6s passed closed gate loop\n", plate);
    // pthread_cond_signal(&carpark.data->entrance[random_entry].)
    // pthread_mutex_unlock(&carpark.data->entrance[random_entry].gate.mutex);
    
    // Give licence plate to LPR and signal manager to check it
    pthread_mutex_lock(&carpark.data->entrance[random_entry].LPR.mutex);
    string2charr(plate, carpark.data->entrance[random_entry].LPR.plate);
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].LPR.mutex);
    pthread_cond_signal(&carpark.data->entrance[random_entry].LPR.condition);

    // Wait for the display to update and read its value
    pthread_mutex_lock(&carpark.data->entrance[random_entry].sign.mutex);
    while (carpark.data->entrance[random_entry].sign.display == EMPTY_SIGN)
    {
        printf("%.6s is waiting for display to update\n", plate);
        pthread_cond_wait(&carpark.data->entrance[random_entry].sign.condition, &carpark.data->entrance[random_entry].sign.mutex);
    }
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].sign.mutex);

    char display = carpark.data->entrance[random_entry].sign.display;
    printf("%.6s read the sign saying %c\n", plate, display);

    // If denied entry drive off
    int level = (int)display - 48;
    if(!(0 < level) || !(level < 6))
    {
        printf("%.6s denied entry so it drives off...\n", plate);

        ms_pause(10);

        pthread_mutex_lock(&entry_mutex[random_entry]);
        entry_list[random_entry] = node_delete(entry_list[random_entry], plate);
        pthread_mutex_unlock(&entry_mutex[random_entry]);
        pthread_cond_broadcast(&entry_cond[random_entry]);

        return NULL;
    }
    level = level - 1;
    // Wait for boomgate to open
    pthread_mutex_lock(&carpark.data->entrance[random_entry].gate.mutex);
    while (carpark.data->entrance[random_entry].gate.status != OPEN)
    {
        printf("%.6s is waiting for gate to open. It is %c\n", plate, carpark.data->entrance[random_entry].gate.status);
        pthread_cond_wait(&carpark.data->entrance[random_entry].gate.condition, &carpark.data->entrance[random_entry].gate.mutex);
    }
    pthread_cond_signal(&carpark.data->entrance[random_entry].gate.condition);
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].gate.mutex);
    
    // Remove car from line and signal next car
    printf("%.6s entered car park\n", plate);
    pthread_mutex_lock(&entry_mutex[random_entry]);
    entry_list[random_entry] = node_delete(entry_list[random_entry], plate);
    pthread_mutex_unlock(&entry_mutex[random_entry]);
    pthread_cond_broadcast(&entry_cond[random_entry]);

    car = htab_find(&verified_cars, plate);
    car->in_carpark = true;

    // Travel to level
    ms_pause(10); // could be after level lpr

    printf("about to trigger level lpr once\n");
    // Trigger level LPR
    pthread_mutex_lock(&carpark.data->level[level].LPR.mutex);
    while(!plates_equal(carpark.data->level[level].LPR.plate, EMPTY_LPR))
    {
        pthread_cond_wait(&carpark.data->level[level].LPR.condition, &carpark.data->level[level].LPR.mutex);
    }
    string2charr(plate, carpark.data->level[level].LPR.plate);
    pthread_cond_signal(&carpark.data->level[level].LPR.condition);
    pthread_mutex_unlock(&carpark.data->level[level].LPR.mutex);
    printf("passed level lpr once\n");
    
    // Park at the carpark for randtime or until alarm goes off
    int park_time = (rand() % 9901) + 100;
    // int park_time = 100;

    
    int park_i = 0;
    while (!carpark.data->level[0].temperature.alarm && (park_i < park_time))
    {
        ms_pause(2);
        park_i += 2;
    }    


    // Trigger level LPR
    pthread_mutex_lock(&carpark.data->level[level].LPR.mutex);
    while(!plates_equal(carpark.data->level[level].LPR.plate, EMPTY_LPR))
    {
        pthread_cond_wait(&carpark.data->level[level].LPR.condition, &carpark.data->level[level].LPR.mutex);
    }
    string2charr(plate, carpark.data->level[level].LPR.plate);
    pthread_cond_signal(&carpark.data->level[level].LPR.condition);
    pthread_mutex_unlock(&carpark.data->level[level].LPR.mutex);


    // Add car to exit list
    pthread_mutex_lock(&exit_mutex[random_exit]);
    exit_list[random_exit] = node_add(exit_list[random_exit], plate);;   
    printf("%.6s joined the line for exit %d\n", exit_list[random_exit]->plate, random_exit + 1);
    pthread_mutex_unlock(&exit_mutex[random_exit]); 

    // Wait in line until next to boomgate
    pthread_mutex_lock(&exit_mutex[random_exit]);
    while(node_find_name(exit_list[random_exit], plate)->next != NULL){
        pthread_cond_wait(&exit_cond[random_exit], &exit_mutex[random_exit]);
    } 
    pthread_mutex_unlock(&exit_mutex[random_exit]);
    
    // Wait at boomgate until it is fully closed
    printf("%.6s arrived at boomgate %d\n", plate, random_exit + 1);
    // pthread_mutex_lock(&carpark.data->exit[random_exit].gate.mutex);
    // while(carpark.data->exit[random_exit].gate.status != CLOSED)
    // {
    //     printf("%.6s waiting on gate to close. gate is %c\n", plate, carpark.data->exit[random_exit].gate.status);
    //     pthread_cond_wait(&carpark.data->exit[random_exit].gate.condition, &carpark.data->exit[random_exit].gate.mutex);
    // }
    // pthread_mutex_unlock(&carpark.data->exit[random_exit].gate.mutex);

    // Give licence plate to lpr

    pthread_mutex_lock(&carpark.data->exit[random_exit].LPR.mutex);
    while(!plates_equal(carpark.data->exit[random_exit].LPR.plate, EMPTY_LPR))
        pthread_cond_wait(&carpark.data->exit[random_exit].LPR.condition, &carpark.data->exit[random_exit].LPR.mutex);
    string2charr(plate, carpark.data->exit[random_exit].LPR.plate);
    pthread_mutex_unlock(&carpark.data->exit[random_exit].LPR.mutex);
    pthread_cond_broadcast(&carpark.data->exit[random_exit].LPR.condition);

    // Wait for boomgate to open
    pthread_mutex_lock(&carpark.data->exit[random_exit].gate.mutex);
    while (carpark.data->exit[random_exit].gate.status != OPEN)
    {
        printf("%.6s is waiting for gate to open. It is %c\n", plate, carpark.data->exit[random_exit].gate.status);
        pthread_cond_wait(&carpark.data->exit[random_exit].gate.condition, &carpark.data->exit[random_exit].gate.mutex);
    }
    pthread_cond_signal(&carpark.data->exit[random_exit].gate.condition);
    pthread_mutex_unlock(&carpark.data->exit[random_exit].gate.mutex);
    
    // Remove car from line and signal next car
    printf("%.6s exited car park\n", plate);
    pthread_mutex_lock(&exit_mutex[random_exit]);
    exit_list[random_exit] = node_delete(exit_list[random_exit], plate);
    pthread_mutex_unlock(&exit_mutex[random_exit]);
    pthread_cond_broadcast(&exit_cond[random_exit]);

    car = htab_find(&verified_cars, plate);
    car->in_carpark = false;

    return NULL;
}

// add plate to linked list
node_t *node_add(node_t *head, char *plate)
{
    node_t *newNode = malloc(sizeof(node_t));
    if (newNode == NULL)
    {
        return NULL;
    }    
    newNode->plate = plate;
    newNode->next = head;
    if (newNode == newNode->next)
    {
        newNode->next = NULL;
    }    
    return newNode;
}

// find car by plate
node_t *node_find_name(node_t *head, char *plate)
{
    for ( ; head != NULL; head = head->next)
    {
        // printf("freeze here maybe?\n");
        if (head->plate != NULL)
        {
            if (strcmp(head->plate, plate) == 0)
            {
                return head;
            }  
        }              
    }
    return NULL;    
}

node_t *node_find_name_array(node_t **node_array, char *plate, int array_len){
    // printf("searching all lists once\n");
    for (int i = 0; i < array_len; i++)
    {
        // printf("no way does it get stuck here right?\n");
        if (node_find_name(node_array[i], plate) != NULL)
        {
            // printf("%.6s\n", plate);
            // printf("found dupe\n");
            return node_array[i];
        }
        // printf("passed node find if\n");
    }
    // printf("returned NULL\n");
    return NULL;    
}

// delete node and free malloc by plate
node_t *node_delete(node_t *head, char *plate)
{
    if (strcmp(head->plate, plate) == 0)
    {
        node_t* newHead = head->next;
        // printf("node is end and start so NULL right? -> %p\n", newHead);
        // if (head != NULL)
        // {
        //     printf("nothing should be waiting on %.6s\n", head->plate);
        // }        
        free(head);
        return newHead;
    }
    node_t *del = node_find_name(head, plate);
    // printf("this should always be NULL -> %p\n", del->next);
    node_t *temp = head;
    for (; head != NULL; head = head->next)
    {
        if (head->next == del)
        {
            head->next = del->next;
            free(del);
        }        
    }
    return temp;
}

int get_plate_count(){
    FILE* input_file = fopen("plates.txt", "r");

    if (input_file == NULL)
    {
        fprintf(stderr, "Error: failed to open file %s", "plates.txt");
        exit(1);
    }
    plate_count = 0;
    char plate[PLATE_LENGTH];
    while( fscanf(input_file, "%s", plate) != EOF )
    {
        plate_count++;
    }
    fclose(input_file);

    return plate_count;
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
        park->entrance[i].gate.status = CLOSED;
        pthread_mutex_init(&park->entrance[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].gate.condition, &cond_attr);


        string2charr(EMPTY_LPR, park->entrance[i].LPR.plate);
        pthread_mutex_init(&park->entrance[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].LPR.condition, &cond_attr);

        park->entrance[i].sign.display = EMPTY_SIGN;
        pthread_mutex_init(&park->entrance[i].sign.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].sign.condition, &cond_attr);        

        // EXITS
        park->exit[i].gate.status = CLOSED;
        pthread_mutex_init(&park->exit[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].gate.condition, &cond_attr);

        string2charr(EMPTY_LPR, park->exit[i].LPR.plate);
        pthread_mutex_init(&park->exit[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].LPR.condition, &cond_attr);

        // LEVELS
        string2charr(EMPTY_LPR, park->level[i].LPR.plate);
        pthread_mutex_init(&park->level[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->level[i].LPR.condition, &cond_attr);

        park->level[i].temperature.alarm = 0;
        park->level[i].temperature.sensor = 0;
        
    }
}
