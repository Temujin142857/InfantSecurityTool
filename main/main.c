#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>


#define NUM_TASKS 13
#define NUM_MONITORS 6

typedef enum {
    ETEMPURATURE = 0,
    ITEMPURATURE,
    BRIGHTNESS,
    HEARTBEAT,
    CO2,
    HUMIDITY,
    LED_NC,
    LED_C,
    LCD,
    ALARM,
    APP,
	ERRHANDLER,
	ERRDECREMENTER,
} TaskIds;

typedef enum {    
    BRIGHTNESS_P=1,    
    HUMIDITY_P,
    LED_NC_P,
   	LCD_P, 
   	ETEMPURATURE_P,  
    APP_P,
   	ERRDECREMENTER_P,
	ERRHANDLER_P,
	ITEMPURATURE_P,
	CO2_P,
	HEARTBEAT_P,	
	LED_C_P,	   
	ALARM_P,
} TaskPrioroties;

const int tasksInPriorityOrder[NUM_TASKS]={ALARM, LED_C, HEARTBEAT, CO2, ITEMPURATURE, ERRHANDLER, ERRDECREMENTER, APP, ETEMPURATURE, LCD, LED_NC, HUMIDITY, BRIGHTNESS};
const uint32_t delayInMillisecondsForMonitorTasks[NUM_MONITORS]={20000, 15000, 30000, 40, 1000, 30000};

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
static volatile float eTempurature;
static SemaphoreHandle_t eTemputatureLock;
static volatile float iTempurature;
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

//error handling variables
//errFlagSignal is a queue in case multiple systems fail simultaneously
static SemaphoreHandle_t errFlagSignal;
static volatile uint8_t errType[11];
static uint8_t errLocationTracker[]={0,0,0,0,0,0,0,0,0,0,0};

static QueueHandle_t errToDecrementQueue;
StaticQueue_t errToDecrementQueueBuffer;
uint8_t errQueueStorage[sizeof(uint8_t) * 33];
//-1 undefined, 1 impossible reading


static SemaphoreHandle_t recoverySignals[NUM_TASKS];
//this is a more critical version, task stays using the cpu even when blocked
//static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;

void panic(){
	//something that shouldn't be possible happened
}

void errDecrementerNC(void *pvParameter){
	uint8_t taskNum;	
	xQueueReceive(errToDecrementQueue, &taskNum, portMAX_DELAY);
	vTaskDelay(delayInMillisecondsForMonitorTasks[taskNum]);
	errLocationTracker[taskNum]--;
	if(errLocationTracker[taskNum]<0){
		errLocationTracker[taskNum]=0;
		panic();
	}
	vTaskDelete( NULL );	
}

void errHandlerC(void *pvParameter){
	
	for(;;){
		xSemaphoreTake(errFlagSignal, portMAX_DELAY);
		for (uint8_t i=0; i<NUM_TASKS; i++) {
			if(errType[tasksInPriorityOrder[i]]>0){
				errLocationTracker[tasksInPriorityOrder[i]]++;
				//start a delay to decrement it
				xQueueSend(errToDecrementQueue, ( void * ) &tasksInPriorityOrder[i], 0);
				xTaskCreatePinnedToCore(errDecrementerNC, NULL, 4096, NULL, ERRDECREMENTER_P, NULL, TASK_CORE_NC);
				
				if(errLocationTracker[tasksInPriorityOrder[i]]>=3){
					//do something when it fails three times within a certain time frame
				}
				xSemaphoreGive(recoverySignals[tasksInPriorityOrder[i]]);
			}
		}	
	}
	vTaskDelete( NULL );
}

void waitRecoveryNC(uint8_t i){
	xSemaphoreTake(recoverySignals[i], portMAX_DELAY);
}

void waitRecoveryC(uint8_t i){
	xSemaphoreTake(recoverySignals[i], portMAX_DELAY);
}

//Monitoring tasks
void eTempratureMonitorNC( void *pvParameters )
{
	
    for( ;; )
    {
		uint8_t errCount=0;
		float tempTempurature=0;
		for(uint8_t i=0;i<3;i++){			
			//read tempurature
			//convert from k to c maybe
			float t=0;
			if(t<EXTERNAL_TEMPUTATURE_FLOOR||t>EXTERNAL_TEMPURATURE_CIEL){				
				if(errCount>=3){
					xSemaphoreGive(errFlagSignal);
					errType[ETEMPURATURE]=1;
					waitRecoveryNC(ETEMPURATURE);
					break;
				}	
				i--;
				errCount++;
			}
			//add to tempTempurature
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

void iTempratureMonitorC( void *pvParameters )
{
	uint8_t errCount=0;
    for( ;; )
    {
		float tempTempurature=0;
		for(uint8_t i=0;i<3;i++){
			//read tempurature
			//convert from k to c maybe
			float t=0;
			if(t<INTERNAL_TEMPURATURE_FLOOR||t>INTERNAL_TEMPURATURE_CIEL){
				i--;
				errCount++;
				if(errCount>3){
					xSemaphoreGive(errFlagSignal);
					errType[1]=1;
					tempTempurature=300;
					break;
				}	
			}
			//add to tempTempurature
		}
		if(tempTempurature==300){	
			waitRecoveryC(1)	;	
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
		
		brightness=10;
		xSemaphoreGive(brightnessLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}


void heartbeatMonitorC( void *pvParameters )
{
    for( ;; )
    {
		heartbeat=10;
		xSemaphoreGive(heartbeatLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}

void CO2MonitorC( void *pvParameters )
{
    for( ;; )
    {
		CO2Level=10;
		xSemaphoreGive(CO2LevelLock);
		vTaskDelay(10000);
    }
    vTaskDelete( NULL );
}

void humidityMonitorNC( void *pvParameters )
{
    for( ;; )
    {
		humidity=10;
		xSemaphoreGive(humidityLock);
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



void app_main(void)
{
	eTemputatureLock = xSemaphoreCreateMutex();
	iTemputatureLock = xSemaphoreCreateMutex();
	brightnessLock = xSemaphoreCreateMutex();
	heartbeatLock = xSemaphoreCreateMutex();
	CO2LevelLock = xSemaphoreCreateMutex();
	humidityLock = xSemaphoreCreateMutex();
	motionLock = xSemaphoreCreateMutex();
	errFlagSignal = xSemaphoreCreateCounting(11,0);	
	errToDecrementQueue = xQueueCreateStatic( 33, sizeof( uint8_t ), &(errQueueStorage[0]), &errToDecrementQueueBuffer);
	for (int i = 0; i < NUM_TASKS; i++){
		recoverySignals[i] = xSemaphoreCreateBinary();
	} 
	
	xTaskCreatePinnedToCore(errHandlerC, NULL, 4096, NULL, ERRHANDLER_P, NULL, TASK_CORE_C);	
	
	xTaskCreatePinnedToCore(eTempratureMonitorNC, NULL, 4096, NULL, ETEMPURATURE_P, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(iTempratureMonitorC, NULL, 4096, NULL, ITEMPURATURE_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(brightnessMonitorNC, NULL, 4096, NULL, BRIGHTNESS_P, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(heartbeatMonitorC, NULL, 4096, NULL, HEARTBEAT_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(CO2MonitorC, NULL, 4096, NULL, CO2_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(humidityMonitorNC, NULL, 4096, NULL, HUMIDITY_P, NULL, TASK_CORE_NC);
	
	xTaskCreatePinnedToCore(lcdControllerNC, NULL, 4096, NULL, LCD_P, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerNC, NULL, 4096, NULL, LED_NC_P, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerC, NULL, 4096, NULL, LED_C_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(alarmControllerC, NULL, 4096, NULL, ALARM_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(appControllerNC, NULL, 4096, NULL, APP_P, NULL, TASK_CORE_NC);
		
	while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}

