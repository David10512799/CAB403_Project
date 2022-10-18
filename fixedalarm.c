#include "carpark.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30
#define RATE_OF_RISE 8
#define HIGH_TEMP 58

typedef struct alarm alarm_t;
struct alarm {
    temperature_t *temperature;
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    int *status;
};

void *tempmonitor(void *arg);


int main(int argc, char **argv){

    
    // Get shared memory object
    shared_carpark_t carpark;
    get_carpark(&carpark);

    pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t alarm_condvar = PTHREAD_COND_INITIALIZER;
    int alarm = 0;
    
    // Monitor the temperature sensors
    for( int i = 0; i < NUM_LEVELS; i++){
        pthread_t sensor_thread;
        alarm_t sensor_alarm;
        sensor_alarm.temperature = &carpark.data->level[i].temperature.sensor;
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

    // Set all alarms in shared memory to active
    for( int i = 0; i < NUM_LEVELS; i++){
        carpark.data->level[i].temperature.alarm = 1;
    }

    pthread_mutex_unlock(&alarm_mutex);

    // Raise all boom gates


    //

    return 0;
}



void *tempmonitor(void *arg)
{

    alarm_t *alarm = (alarm_t *)arg;
    temperature_t *temperature = &alarm->temperature;


    int16_t raw_temp[TEMPCHANGE_WINDOW];
    int16_t smooth_temp[TEMPCHANGE_WINDOW];
    int16_t median_list[MEDIAN_WINDOW];

    int raw_count = 0;
    int smooth_count = 0;

	while(!alarm->status) {
		
        // RAW TEMPERATURE READINGS
        if ( raw_count < TEMPCHANGE_WINDOW) 
        {
            raw_temp[raw_count] = temperature->sensor;
            raw_count++;
        }
		else
        {
            assert(raw_count == TEMPCHANGE_WINDOW);

            // Shuffle temperatures toward beginning of the array and insert new value at end
            for( int i = 0; i < TEMPCHANGE_WINDOW - 1; i++){
                raw_temp[i] = raw_temp[i+1];
            }
            raw_temp[TEMPCHANGE_WINDOW - 1] = temperature->sensor; 
        }

        // GENERATE SMOOTH TEMPERATURES
        if ( raw_count >= MEDIAN_WINDOW )
        {
            // insert last five values of raw temp
            for( int i = 0; i < MEDIAN_WINDOW; i++){
                median_list[i] = raw_temp[raw_count - 1 - i];
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
            if ( smooth_count < TEMPCHANGE_WINDOW) 
            {
                smooth_temp[smooth_count] = median_list[(MEDIAN_WINDOW - 1) / 2];
                smooth_count++;
            }
            else
            {
                assert(smooth_count == TEMPCHANGE_WINDOW);

                // Shuffle temperatures toward beginning of the array and insert new value at end
                for( int i = 0; i < TEMPCHANGE_WINDOW - 1; i++){
                    smooth_temp[i] = smooth_temp[i+1];
                }
                smooth_temp[TEMPCHANGE_WINDOW - 1] = median_list[(MEDIAN_WINDOW - 1) / 2]; 
            }            

        }

        if ( smooth_count == TEMPCHANGE_WINDOW )
        {
            // RATE-OF-RISE FIRE DETECTION
            if (smooth_temp[TEMPCHANGE_WINDOW - 1] - smooth_temp[0] >= RATE_OF_RISE)
            {
                // Activate alarm
                pthread_mutex_lock(&alarm->mutex);
                alarm->status = 1;
                pthread_cond_signal(&alarm->condition);
                pthread_mutex_unlock(&alarm->mutex);
            }

            // FIXED TEMPERATURE FIRE DETECTION
            int high_count = 0;
            for( int i = 0; i < TEMPCHANGE_WINDOW; i++){
                if (smooth_temp[i] >= HIGH_TEMP)
                {
                    high_count++;
                }
            }
            if ( high_count >= TEMPCHANGE_WINDOW * 0.9 )
            {
                pthread_mutex_lock(&alarm->mutex);
                alarm->status = 1;
                pthread_cond_signal(&alarm->condition);
                pthread_mutex_unlock(&alarm->mutex);
            }
        }
		
		usleep(2000);
		
	}
}