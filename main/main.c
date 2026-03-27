#include "freertos/idf_additions.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>


//NOTE 0 IS THE LOWEST PRIORITY
//priorities of tasks
#define TASK_PRIO_NC_MONITOR 1
#define TASK_PRIO_C_MONITOR 2

//Which core to pin the tasks to
#define TASK_CORE_NC 0
#define TASK_CORE_C 1

//data variables and semiphores
static volatile int tempurature;
static SemaphoreHandle_t temputatureLock;
static volatile int brightness;
static SemaphoreHandle_t brightnessLock;
static volatile int heartbeat;
static SemaphoreHandle_t heartbeatLock;
static volatile int CO2Level;
static SemaphoreHandle_t CO2LevelLock;
static volatile int humidity;
static SemaphoreHandle_t humidityLock;
//motion will probably be an array?
static volatile int motion;
static SemaphoreHandle_t motionLock;


//this is a more critical version, task stays using the cpu even when blocked
//static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;



//Monitoring tasks
void tempratureMonitorNC( void *pvParameters )
{
    for( ;; )
    {
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}


void brightnessMonitorNC( void *pvParameters )
{
    for( ;; )
    {
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}


void heartbeatMonitorC( void *pvParameters )
{
    for( ;; )
    {
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}

void CO2MonitorC( void *pvParameters )
{
    for( ;; )
    {
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}

void humidityMonitorNC( void *pvParameters )
{
    for( ;; )
    {
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}

//end of monitor tasks

//ui level tasks
void ledControllerNC(void *pvParameter){
	
	for( ;; )
	{
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
	}
	vTaskDelete( NULL );
}

void ledControllerC(void *pvParameter){

	for( ;; )
	{
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
	}
	vTaskDelete( NULL );
}

void lcdControllerNC(void *pvParameter){

	for( ;; )
	{
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
	}
	vTaskDelete( NULL );
}

void alarmControllerC(void *pvParameter){

	for( ;; )
	{
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
	}
	vTaskDelete( NULL );
}

void app_main(void)
{
	temputatureLock = xSemaphoreCreateMutex();
	brightnessLock = xSemaphoreCreateMutex();
	heartbeatLock = xSemaphoreCreateMutex();
	CO2LevelLock = xSemaphoreCreateMutex();
	humidityLock = xSemaphoreCreateMutex();
	motionLock = xSemaphoreCreateMutex();
	
	
	xTaskCreatePinnedToCore(tempratureMonitorNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(brightnessMonitorNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(heartbeatMonitorC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(CO2MonitorC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(humidityMonitorNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	
	while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}

