#include "carpark.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30

typedef struct alarm alarm_t;
struct alarm {
    temperature_t *temperature;
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    gate_t *gate;
    sign_t *sign;
    int *status;
};

static void *tempmonitor(void *arg);
static void *open_gate(void *arg);
static void *display_evac(void *arg);
static void read_temps(temperature_t *temperature, int16_t *temp, int *count);
static void generate_smooth(int16_t *raw_temp, int16_t *smooth_temp, int16_t *median_list, int *raw_count, int *smooth_count);
static void detect_fire(int16_t *smooth_temp, int *smooth_count, alarm_t *alarm);
static void detect_hardware_failure(int16_t *raw_temp, int raw_count, alarm_t *alarm);

int main(void){

    
    // Get shared memory object
    shared_carpark_t carpark;
    carpark.name = SHARE_NAME;
    carpark.fd = shm_open(SHARE_NAME, O_RDWR, 438);
    assert(carpark.fd != -1);
    carpark.data = mmap(0, sizeof(carpark_t),PROT_READ | PROT_WRITE, MAP_SHARED, carpark.fd, 0);
    assert(carpark.data != (void *)-1);

    // Initialise data for alarm status local to firealarm.c
    pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t alarm_condvar = PTHREAD_COND_INITIALIZER;
    int alarm = 0;
    
    // Monitor the temperature sensors
    for( int i = 0; i < LEVELS; i++){
        pthread_t sensor_thread;
        alarm_t sensor_alarm;
        sensor_alarm.temperature = &carpark.data->level[i].temperature;
        sensor_alarm.mutex = &alarm_mutex;
        sensor_alarm.condition = &alarm_condvar;
        sensor_alarm.status = &alarm;
        pthread_create(&sensor_thread, NULL, tempmonitor, &sensor_alarm);
    }

    // Wait on active alarm
    pthread_mutex_lock(&alarm_mutex);
    while (!alarm)
    {
        pthread_cond_wait(&alarm_condvar, &alarm_mutex);
    }
    printf("passed alarm wait\n");

    // Set all alarms in shared memory to active
    for( int i = 0; i < LEVELS; i++){
        carpark.data->level[i].temperature.alarm = 1;
        printf("level %d alarm is %d\n", i + 1, carpark.data->level[i].temperature.alarm);
    }
    pthread_mutex_unlock(&alarm_mutex);
    printf("alarms set to active\n");

    // Raise all entrance gates and display evacuate on signs
    pthread_t entry_gates[ENTRIES];
    pthread_t signs[ENTRIES];

    for( int i = 0; i < ENTRIES; i++){
        alarm_t gate_alarm;
        gate_alarm.gate = &carpark.data->entrance[i].gate;
        gate_alarm.status = &alarm;
        pthread_create(&entry_gates[i], NULL, open_gate, &gate_alarm);

        alarm_t sign_alarm;
        sign_alarm.sign = &carpark.data->entrance[i].sign;
        sign_alarm.status = &alarm;
        pthread_create(&signs[i], NULL, display_evac, &sign_alarm);
    }
    printf("raise entry\n");
    // Raise all exit gates
    pthread_t exit_gates[EXITS];
    for( int i = 0; i < EXITS; i++){
        alarm_t exit_alarm;
        exit_alarm.gate = &carpark.data->exit[i].gate;
        exit_alarm.status = &alarm;
        pthread_create(&exit_gates[i], NULL, open_gate, &exit_alarm);
    }
    printf("raise exit\n");
    
    // Wait for manager to turn off alarms once all cars have exited;
    while(carpark.data->level[0].temperature.alarm == 1)
    {
        printf("waiting\n");
        sleep(5);
    }

    // Allow threads to terminate
    for( int i = 0; i < ENTRIES; i++){
        pthread_join(entry_gates[i], NULL);
        pthread_join(signs[i], NULL);
    }

    for( int i = 0; i < EXITS; i++){
        pthread_join(exit_gates[i], NULL);
    }
    printf("after joining threads\n");

    // Unmap memory before the program closes
    munmap(&carpark.data, sizeof(carpark_t));
    carpark.data = NULL;
    carpark.fd = -1;

    return 0;
}



static void read_temps(temperature_t *temperature, int16_t *temp, int *count)
{
    if ( *count < TEMPCHANGE_WINDOW ) 
    {
        temp[*count] = temperature->sensor;
        *count = *count + 1;
    }
    else
    {
        assert(*count == TEMPCHANGE_WINDOW);

        // Shuffle temperatures toward beginning of the array and insert new value at end
        for( int i = 0; i < (*count - 1); i++){
            temp[i] = temp[i+1];
        }
        temp[*count - 1] = temperature->sensor; 
    }
}

static void generate_smooth(int16_t *raw_temp, int16_t *smooth_temp, int16_t *median_list, int *raw_count, int *smooth_count)
{
    // insert last five values of raw temp
    for( int i = 0; i < MEDIAN_WINDOW; i++){
        median_list[i] = raw_temp[*raw_count - 1 - i];
    }
    // sort
    for (int i = 0; i < MEDIAN_WINDOW; ++i) 
    {
        for (int j = i + 1; j < MEDIAN_WINDOW; ++j)
        {
            if (median_list[i] > median_list[j]) 
            {
                int temp =  median_list[i];
                median_list[i] = median_list[j];
                median_list[j] = temp;
            }
        }
    }

    // insert median into smooth_temp
    if ( *smooth_count < TEMPCHANGE_WINDOW) 
    {
        smooth_temp[*smooth_count] = median_list[(MEDIAN_WINDOW - 1) / 2];
        *smooth_count = *smooth_count + 1;
    }
    else
    {
        assert(*smooth_count == TEMPCHANGE_WINDOW);

        // Shuffle temperatures toward beginning of the array and insert new value at end
        for( int i = 0; i < (*smooth_count - 1); i++){
            smooth_temp[i] = smooth_temp[i+1];
        }
        smooth_temp[*smooth_count - 1] = median_list[(MEDIAN_WINDOW - 1) / 2]; 
    }  
}

static void detect_fire(int16_t *smooth_temp, int *smooth_count, alarm_t *alarm)
{
    // RATE-OF-RISE FIRE DETECTION
    if ((smooth_temp[*smooth_count - 1] - smooth_temp[0]) >= 8)
    {
        printf("rate of rise alarm triggered\n");
        // Activate alarm
        pthread_mutex_lock(alarm->mutex);
        *alarm->status = 1;
        pthread_cond_signal(alarm->condition);
        pthread_mutex_unlock(alarm->mutex);
    }

    // FIXED TEMPERATURE FIRE DETECTION
    int high_count = 0;
    for( int i = 0; i < TEMPCHANGE_WINDOW; i++){
        if (smooth_temp[i] >= 58)
        {
            high_count++;
        }
    }
    if ( high_count >= (*smooth_count * 0.9) )
    {
        printf("fixed temperature alarm triggered\n");
        pthread_mutex_lock(alarm->mutex);
        *alarm->status = 1;
        pthread_cond_signal(alarm->condition);
        pthread_mutex_unlock(alarm->mutex);
    }    
}

static void *tempmonitor(void *arg)
{

    alarm_t *alarm = (alarm_t *)arg;
    temperature_t *temperature = alarm->temperature;

    int16_t raw_temp[TEMPCHANGE_WINDOW];
    int16_t smooth_temp[TEMPCHANGE_WINDOW];
    int16_t median_list[MEDIAN_WINDOW];

    int raw_count = 0;
    int smooth_count = 0;
    printf("alarm status is %d?\n", *alarm->status);


	while(!*alarm->status) {
        printf("temp is %d\n", temperature->sensor);
		
        // RAW TEMPERATURE READINGS
        read_temps(temperature, raw_temp, &raw_count);

        detect_hardware_failure(raw_temp, raw_count, alarm);

        // GENERATE SMOOTH TEMPERATURES
        if ( raw_count >= MEDIAN_WINDOW )
        {
            generate_smooth(raw_temp, smooth_temp, median_list, &raw_count, &smooth_count);          
        }
        // DETECT FIRES
        if ( smooth_count == TEMPCHANGE_WINDOW )
        {
            detect_fire(smooth_temp, &smooth_count, alarm);
        }
        printf("alarm status is %d\n", *alarm->status);

		usleep(2000);
	}
    return NULL;
}

static void *open_gate(void *arg)
{
    alarm_t *alarm = (alarm_t *)arg;
	gate_t *bg = alarm->gate;

	pthread_mutex_lock(&bg->mutex);
	while (*alarm->status == 1) {
		if (bg->status == (char)CLOSED) {
			bg->status = RAISING;
			pthread_cond_broadcast(&bg->condition);
		}
		pthread_cond_wait(&bg->condition, &bg->mutex);
	}
	pthread_mutex_unlock(&bg->mutex);
    return NULL;
}

static void *display_evac(void *arg)
{
    alarm_t *alarm = (alarm_t *)arg;
    sign_t *sign = alarm->sign;
    const char *evacmessage = "EVACUATE ";

    while (*alarm->status == 1) {
        int i = 0;
        while (evacmessage[i] != '\0')
        {
            pthread_mutex_lock(&sign->mutex);
            sign->display = evacmessage[i];
            pthread_cond_broadcast(&sign->condition);
            pthread_mutex_unlock(&sign->mutex);
			usleep(20000);
        }
    }
    return NULL;
}

static void detect_hardware_failure(int16_t *raw_temp, int raw_count, alarm_t *alarm)
{
    int bad_value_count = 0;
    int consecutive_count = 0;

    for( int i = 0; i < raw_count; i++){
        if (raw_temp[i] > 100)
        {
            bad_value_count++;
        }
        if (raw_temp[i] < -23)
        {
            bad_value_count++;
        }
    }

    if (raw_count == TEMPCHANGE_WINDOW )
    {
        for( int i = 1; i < raw_count; i++){
            if(raw_temp[i] == raw_temp[i - 1])
            {
                consecutive_count++;
            }
            else
            {
                consecutive_count = 0;
            }
        }
    }

    if ((bad_value_count > 2) || (consecutive_count == TEMPCHANGE_WINDOW - 1))
    {
        printf("hardware failure alarm triggered\n");
        pthread_mutex_lock(alarm->mutex);
        *alarm->status = 1;
        pthread_cond_signal(alarm->condition);
        pthread_mutex_unlock(alarm->mutex);
    }

}