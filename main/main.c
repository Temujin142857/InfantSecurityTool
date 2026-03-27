#include "freertos/idf_additions.h"
#include "portmacro.h"
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

//dataDictionary constants
#define EXTERNAL_TEMPURATURE_MAX 22.2
#define EXTERNAL_TEMPURATURE_MIN 20
#define EXTERNAL_TEMPURATURE_CIEL 50
#define EXTERNAL_TEMPUTATURE_FLOOR -40

#define INTERNAL_TEMPURATURE_MAX 37.5
#define INTERNAL_TEMPURATURE_MIN 36
#define INTERNAL_TEMPURATURE_CIEL 50
#define INTERNAL_TEMPURATURE_FLOOR 20

//not sure what these will look like yet
#define CO2_LEVEL_MAX 1
#define HUMIDITY_MAX 0.55
#define HEARTBEAT_MIN 1
#define HEARTBEAT_MAX 1

//data variables and semiphores
static volatile int eTempurature;
static SemaphoreHandle_t eTemputatureLock;
static volatile int iTempurature;
static SemaphoreHandle_t iTemputatureLock;
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

//error handling varaibles
static volatile int errFlag;
static SemaphoreHandle_t errFlagSignal;
static volatile int errType;
static volatile int errLocation;
//1. Etempurature 2. ITempurature 3. 

static SemaphoreHandle_t recoverySignalNC;
static SemaphoreHandle_t recoverySignalC;
//this is a more critical version, task stays using the cpu even when blocked
//static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;



//Monitoring tasks
void eTempratureMonitorNC( void *pvParameters )
{
	int errCount=0;
    for( ;; )
    {
		float tempTempurature=0;
		for(int i=0;i<3;i++){
			//read tempurature
			//convert from k to c maybe
			float t=0;
			if(t<EXTERNAL_TEMPUTATURE_FLOOR||t>EXTERNAL_TEMPURATURE_CIEL){
				i--;
				errCount++;
				if(errCount>3){
					errFlag=1;
					errType=2;
					errLocation=2;
					tempTempurature=300;
					break;
				}	
			}
			//add to tempTempurature
		}
		if(tempTempurature==300){
			xSemaphoreTake(recoverySignalNC, portMAX_DELAY);
		}
		
		tempTempurature=tempTempurature/3;		
		if(tempTempurature>EXTERNAL_TEMPURATURE_MAX){
		} else if (tempTempurature<EXTERNAL_TEMPURATURE_MIN){
		}
		eTempurature=tempTempurature;
		xSemaphoreGive(eTemputatureLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}

void iTempratureMonitorNC( void *pvParameters )
{
	int errCount=0;
    for( ;; )
    {
		float tempTempurature=0;
		for(int i=0;i<3;i++){
			//read tempurature
			//convert from k to c maybe
			float t=0;
			if(t<INTERNAL_TEMPURATURE_FLOOR||t>INTERNAL_TEMPURATURE_CIEL){
				i--;
				errCount++;
				if(errCount>3){
					errFlag=1;
					errType=2;
					errLocation=2;
					tempTempurature=300;
					break;
				}	
			}
			//add to tempTempurature
		}
		if(tempTempurature==300){			
		}
		
		tempTempurature=tempTempurature/3;		
		if(tempTempurature>EXTERNAL_TEMPURATURE_MAX){
		} else if (tempTempurature<EXTERNAL_TEMPURATURE_MIN){
		}
		iTempurature=tempTempurature;
		xSemaphoreGive(iTemputatureLock);
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

void appControllerNC(void *pvParameter){

	for( ;; )
	{
		tempurature=10;
		xSemaphoreGive(temputatureLock);
		vTaskDelay(10000);
	}
	vTaskDelete( NULL );
}

void errHandlerC(void *pvParameter){
	int []
	for(;;){
		xSemaphoreTake(errFlagSignal, portMAX_DELAY);
		
	}
	vTaskDelete( NULL );
}

void app_main(void)
{
	eTemputatureLock = xSemaphoreCreateMutex();
	iTemputatureLock = xSemaphoreCreateMutex();
	brightnessLock = xSemaphoreCreateMutex();
	heartbeatLock = xSemaphoreCreateMutex();
	CO2LevelLock = xSemaphoreCreateMutex();
	humidityLock = xSemaphoreCreateMutex();
	motionLock = xSemaphoreCreateMutex();
	errFlagSignal = xSemaphoreCreateMutex();	
	
	xTaskCreatePinnedToCore(eTempratureMonitorNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(iTempratureMonitorC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(brightnessMonitorNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(heartbeatMonitorC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(CO2MonitorC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(humidityMonitorNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	
	xTaskCreatePinnedToCore(lcdControllerNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(alarmControllerC, NULL, 4096, NULL, TASK_PRIO_C_MONITOR, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(appControllerNC, NULL, 4096, NULL, TASK_PRIO_NC_MONITOR, NULL, TASK_CORE_NC);
		
	while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}

