#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "I2C.h"
#include "max30102.h"
#include "algorithm_by_RF.h"
#include "lcd.h"
#include "led.h"


#define NUM_TASKS 15
#define NUM_MONITORS 8
#define NUM_UI_TASKS 5

typedef enum {
    ETEMPURATURE = 0,
    ITEMPURATURE,
    BRIGHTNESS,
    HEARTBEAT_MONITOR,
	HEARTBEAT_PROCESSOR,
    CO2,
    HUMIDITY,
	MOTION,
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
	MOTION_P,
    APP_P,
   	ERRDECREMENTER_P,
	ERRHANDLER_P,
	ITEMPURATURE_P,
	CO2_P,
	HEARTBEAT_MONITOR_P,
	HEARTBEAT_PROCESSOR_P,	
	LED_C_P,	   
	ALARM_P,
} TaskPrioroties;

const int tasksInPriorityOrder[NUM_TASKS]={ALARM, LED_C, HEARTBEAT_PROCESSOR, HEARTBEAT_MONITOR, CO2, ITEMPURATURE, MOTION, ERRHANDLER, ERRDECREMENTER, APP, ETEMPURATURE, LCD, LED_NC, HUMIDITY, BRIGHTNESS};
const uint32_t delayInMillisecondsForMonitorTasks[NUM_MONITORS]={20000, 15000, 30000, 40, 1000, 30000, 300};


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

//note heartrate and blood oxygen level constants for the raw inputs are taken care of in the algorithm by RF file
#define RED_CIEL 200000
#define RED_FLOOR 5000
#define IR_CIEL 200000
#define IR_FLOOR 5000


//not sure what these will look like yet
#define CO2_LEVEL_MAX 1
#define HUMIDITY_MAX 0.55
#define HEARTBEAT_MIN 1
#define HEARTBEAT_MAX 1

//shared data variables and semiphores
static volatile float eTempurature;
static SemaphoreHandle_t eTempuratureLock;
static volatile float iTempurature;
static SemaphoreHandle_t iTempuratureLock;
static volatile int brightness;
static SemaphoreHandle_t brightnessLock;
static volatile int heartbeat;
static volatile int SO2Level;
static SemaphoreHandle_t heartProcessSignal;
static SemaphoreHandle_t heartLock;
static volatile int CO2Level;
static SemaphoreHandle_t CO2LevelLock;
static volatile int humidity;
static SemaphoreHandle_t humidityLock;
//motion will probably be an array?
static volatile int motion;
static SemaphoreHandle_t motionLock;

//these queues will be how the monitors notify the ui tasks that there is new data
//i.e., these are the ui task inboxes
static QueueHandle_t ledControllerNC_Queue;
static QueueHandle_t ledControllerC_Queue;
static QueueHandle_t lcdControllerNC_Queue;
static QueueHandle_t alarmControllerC_Queue;
static QueueHandle_t appControllerNC_Queue;

StaticQueue_t ledControllerNC_QueueBuffer;
StaticQueue_t ledControllerC_QueueBuffer;
StaticQueue_t lcdControllerNC_QueueBuffer;
StaticQueue_t alarmControllerC_QueueBuffer;
StaticQueue_t appControllerNC_QueueBuffer;

uint8_t ledControllerNC_QueueStorage[sizeof(uint8_t) * NUM_MONITORS];
uint8_t ledControllerC_QueueStorage[sizeof(uint8_t) * NUM_MONITORS];
uint8_t lcdControllerNC_QueueStorage[sizeof(uint8_t) * NUM_MONITORS];
uint8_t alarmControllerC_QueueStorage[sizeof(uint8_t) * NUM_MONITORS];
uint8_t appControllerNC_QueueStorage[sizeof(uint8_t) * NUM_MONITORS];

//assigning the ui task handles in case I want to use notification later
TaskHandle_t ledControllerNC_Handle = NULL;
TaskHandle_t ledControllerC_Handle = NULL;
TaskHandle_t lcdControllerNC_Handle = NULL;
TaskHandle_t alarmControllerC_Handle = NULL;
TaskHandle_t appControllerNC_Handle = NULL;

//heartbeat preocessing variables
#define HEARTBEAT_BUFFER_SIZE 100
uint32_t activeIRSamples[HEARTBEAT_BUFFER_SIZE]; 
uint32_t activeREDSamples[HEARTBEAT_BUFFER_SIZE]; 

//error handling variables
//errFlagSignal is a queue in case multiple systems fail simultaneously
static SemaphoreHandle_t errFlagSignal;
//this is how each task tells the errHandler what kind of error it has
static SemaphoreHandle_t errTypeLock;
static volatile uint8_t errType[11];

static SemaphoreHandle_t errLocationTrackerLock;
static uint8_t errLocationTracker[]={0,0,0,0,0,0,0,0,0,0,0};

static SemaphoreHandle_t errFlagSignalMajor;
static volatile uint8_t majorErrLocationTracker[NUM_TASKS]={0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0};
static SemaphoreHandle_t errMajorTaskWating;
static volatile uint8_t errWatingTracker[NUM_TASKS]={0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0}

static QueueHandle_t errToDecrementQueue;
StaticQueue_t errToDecrementQueueBuffer;
uint8_t errQueueStorage[sizeof(uint8_t) * 3 * NUM_TASKS];
//-1 undefined, 1 impossible reading


static SemaphoreHandle_t recoverySignals[NUM_TASKS];
//this is a more critical version, task stays using the cpu even when blocked
//static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;

// start of error handling tasks
void errDecrementerC(void *pvParameter){
	uint8_t taskNum;	
	xQueueReceive(errToDecrementQueue, &taskNum, portMAX_DELAY);
	vTaskDelay(delayInMillisecondsForMonitorTasks[taskNum]);
	errLocationTracker[taskNum]--;
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
				xTaskCreatePinnedToCore(errDecrementerC, NULL, 4096, NULL, ERRDECREMENTER_P, NULL, TASK_CORE_C);
				
				if(errLocationTracker[tasksInPriorityOrder[i]]>=3){
					//enter error state when it fails three times within a certain time frame
					//revisit when logic for display and lights are more clear					
				} else{
					xSemaphoreGive(recoverySignals[tasksInPriorityOrder[i]]);
				}				
			}
		}	
	}
	vTaskDelete( NULL );
}


//end of error handling tasks

//monitor task helpers
void waitRecoveryNC(uint8_t i){
	xSemaphoreTake(recoverySignals[i], portMAX_DELAY);
}

//return 0 is recovered, return 1 is not
uint8_t waitRecoveryC(uint8_t i){
	if(xSemaphoreTake(recoverySignals[i], 0)){
		return 0;
	}else if(xSemaphoreTake(errFlagSignalMajor, 0)){
		if(majorErrLocationTracker[i]>0){
			xSemaphoreTake(recoverySignals[i], portMAX_DELAY);
			return 0;
		} else{
			return 1;
		}		
	} else {
		return 1;
	}
}

void notifyAllUITasks(uint32_t id){
	xQueueSend(ledControllerNC_Queue, (void *) &id, 0);
	xQueueSend(ledControllerC_Queue, (void *) &id, 0);
	xQueueSend(lcdControllerNC_Queue, (void *) &id, 0);
	xQueueSend(alarmControllerC_Queue, (void *) &id, 0);
	xQueueSend(appControllerNC_Queue, (void *) &id, 0);
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
		xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
		eTempurature=tempTempurature;
		xSemaphoreGive(eTempuratureLock);
		notifyAllUITasks(ETEMPURATURE);
		vTaskDelay(delayInMillisecondsForMonitorTasks[ETEMPURATURE]);
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
		xSemaphoreTake(iTempuratureLock, portMAX_DELAY);
		iTempurature=tempTempurature;
		xSemaphoreGive(iTempuratureLock);
		notifyAllUITasks(ITEMPURATURE);
		vTaskDelay(delayInMillisecondsForMonitorTasks[ITEMPURATURE]);
    }
    vTaskDelete( NULL );
}


void brightnessMonitorNC( void *pvParameters )
{
    for( ;; )
    {		
		xSemaphoreTake(brightnessLock, portMAX_DELAY);
		brightness=10;
		xSemaphoreGive(brightnessLock);
		notifyAllUITasks(BRIGHTNESS);
		vTaskDelay(delayInMillisecondsForMonitorTasks[BRIGHTNESS]);
    }
    vTaskDelete( NULL );
}


void heartbeatMonitorC( void *pvParameters )
{	
	uint8_t errCount=0;
	uint8_t batchCount=0;
	uint8_t badBatchCount=0;
	uint8_t index=0;
	const uint8_t maxSamplesPerReadBatch=8;
	const uint8_t samplesPerProcessingRound=128;
	uint8_t awaitingErrProcessing=0;
	int samplesInBatch;
	TickType_t lastWakeTime = xTaskGetTickCount();
	
    for( ;; )
    {
		if(awaitingErrProcessing){
			awaitingErrProcessing = waitRecoveryC(HEARTBEAT_MONITOR);
		}
		max_sample_t tempSamples[8];
		samplesInBatch = max30102_read_fifo(&tempSamples[0], maxSamplesPerReadBatch);
		for(int i=0;i<samplesInBatch;i++){
			//note we check as a pair, since only including one would desync the indexes of the array
			if(tempSamples[i].ir<IR_FLOOR&&tempSamples[i].ir>IR_FLOOR&&tempSamples[i].red<RED_FLOOR&&tempSamples[i].red>RED_FLOOR){
				activeIRSamples[index+i]=tempSamples[i].ir;
				activeREDSamples[index+i]=tempSamples[i].red;
			} else{
				errCount++;
			}	
		}
		batchCount++;
		if(errCount>=samplesInBatch/2){
			badBatchCount++;
		}		
		errCount=0;
		//note we increment the index regardless of errors, basically it means we reuse the previous cycles mesurements instead of the erronious ones
		index+=samplesInBatch;
		if(index>=samplesPerProcessingRound){
			if(badBatchCount>=batchCount/2){
				errLocationTracker[HEARTBEAT_MONITOR]=1;
				xSemaphoreGive(errFlagSignal);
				awaitingErrProcessing=1;
			}
			xSemaphoreGive(heartProcessSignal); 
			batchCount=0;
			badBatchCount=0;
			index=0;
		}
		//this task requires extreme timing precision to properly calculate bpm, so we use delay until instead of delay
		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(delayInMillisecondsForMonitorTasks[HEARTBEAT_MONITOR]));
    }
    vTaskDelete( NULL );
}


void heartbeatProcessorC(void *pvParameters)
{
    float spo2;
    int8_t spo2_valid;
    int32_t heart_rate;
    int8_t hr_valid;
	float ratio;
	float correl;

    for ( ;; )
    {
        if (xSemaphoreTake(heartProcessLock, portMAX_DELAY))
        {
            // Call Maxim algorithm 
			rf_heart_rate_and_oxygen_saturation(activeIRSamples, HEARTBEAT_BUFFER_SIZE, activeREDSamples, &spo2, &spo2_valid, &heart_rate, &hr_valid, &ratio, &correl);

			if(spo2_valid||hr_valid){
				xSemaphoreTake(heartLock, portMAX_DELAY);
			}	
						
            if (hr_valid)
            {
				printf("Heart Rate: %li bpm\n", (long) heart_rate);
				heartbeat=heart_rate;
            } 

            if (spo2_valid)
            {
            	printf("SpO2: %f %%\n", spo2);
				SO2Level=spo2;
            }   
			
			if(!spo2_valid||!hr_valid){		
				//note no need to block if there's an error, any time the sensor isn't blocked it should try to process the data
				//if there's something wrong with the sensor it should block itself at that level, not needed here		
				errLocationTracker[HEARTBEAT_PROCESSOR]=1;
				xSemaphoreGive(errFlagSignal);
			}		
			
			if(spo2_valid||hr_valid){
				xSemaphoreGive(heartLock);
				notifyAllUITasks(HEARTBEAT_PROCESSOR);
			}			
        }
    }
	vTaskDelete( NULL );
}

void CO2MonitorC( void *pvParameters )
{
    for( ;; )
    {
		xSemaphoreTake(CO2LevelLock, portMAX_DELAY);
		CO2Level=10;
		xSemaphoreGive(CO2LevelLock);
		notifyAllUITasks(CO2);
		vTaskDelay(delayInMillisecondsForMonitorTasks[CO2]);
    }
    vTaskDelete( NULL );
}

void humidityMonitorNC( void *pvParameters )
{
    for( ;; )
    {
		xSemaphoreTake(humidityLock, portMAX_DELAY);
		humidity=10;
		xSemaphoreGive(humidityLock);
		notifyAllUITasks(HUMIDITY);
		vTaskDelay(delayInMillisecondsForMonitorTasks[HUMIDITY]);
    }
    vTaskDelete( NULL );
}

//end of monitor tasks

//ui helpers
//maybe make this the controller, and have it call the other ui tasks
//like it processes the data, and decides who needs to update their values
void readTheSharedData(int id, float *tempTempurature, float *tempBrightness, int *tempHeartbeat, int *tempSO2Level, int *tempCO2Level, int *tempHumidity, int *tempMotion){
	switch (id){
		case ETEMPURATURE:
			xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
			*tempTempurature=eTempurature;
			xSemaphoreGive(eTempuratureLock);
			break;
					
		case ITEMPURATURE:
			xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
			*tempTempurature=iTempurature;
			xSemaphoreGive(eTempuratureLock);
			break;	
					
		case BRIGHTNESS:
			xSemaphoreTake(brightnessLock, portMAX_DELAY);
			*tempBrightness=brightness;
			xSemaphoreGive(brightnessLock);
			break;	
					
		case HEARTBEAT_PROCESSOR:
			xSemaphoreTake(heartLock, portMAX_DELAY);
			*tempHeartbeat=heartbeat;
			*tempSO2Level=SO2Level;
			xSemaphoreGive(heartLock);
			break;	
									
		case CO2:
			xSemaphoreTake(CO2LevelLock, portMAX_DELAY);
			*tempCO2Level=CO2Level;
			xSemaphoreGive(CO2LevelLock);
			break;	
					
		case HUMIDITY:
			xSemaphoreTake(humidityLock, portMAX_DELAY);
			*tempHumidity=humidity;
			xSemaphoreGive(humidityLock);
			break;
									
		case MOTION:
			xSemaphoreTake(motionLock, portMAX_DELAY);
			*tempMotion=motion;
			xSemaphoreGive(motionLock);
			break;														
	}
}


//0 is fine, 1 is warning, 2 is emergency
uint8_t eTempuratureCheck(float tempurature){
	return 0;
}

uint8_t iTempuratureCheck(float tempurature){
	return 0;
}



//ui level tasks
void ledControllerNC(void *pvParameter){
	uint8_t monitorId;
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempCO2Level;
	int tempHumidity;
	int tempMotion;
	
	for( ;; )
	{
		monitorId=xQueueSemaphoreTake(ledControllerNC_Queue, portMAX_DELAY);
		readTheSharedData(monitorId, &tempTempurature, &tempBrightness, &tempHeartbeat, &tempSO2Level, &tempCO2Level, &tempHumidity, &tempMotion);
		gpio_toggle(LED_ORANGE);
	}
	vTaskDelete( NULL );
}

void ledControllerC(void *pvParameter){
	uint32_t monitorTaskID;
	uint8_t monitorId;
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempCO2Level;
	int tempHumidity;
	int tempMotion;
	for( ;; )
	{
		monitorId=xQueueSemaphoreTake(ledControllerC_Queue, portMAX_DELAY);
		readTheSharedData(monitorId, &tempTempurature, &tempBrightness, &tempHeartbeat, &tempSO2Level, &tempCO2Level, &tempHumidity, &tempMotion);
		gpio_toggle(LED_RED);
	}
	vTaskDelete( NULL );
}

void lcdControllerNC(void *pvParameter){
	uint8_t monitorId;
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempCO2Level;
	int tempHumidity;
	int tempMotion;
	char toDisplay[18];
	for( ;; )
	{
		monitorId=xQueueSemaphoreTake(lcdControllerNC_Queue, portMAX_DELAY);
		readTheSharedData(monitorId, &tempTempurature, &tempBrightness, &tempHeartbeat, &tempSO2Level, &tempCO2Level, &tempHumidity, &tempMotion);

		LCD_Clear();
		LCD_SetCursor(0,0);
		LCD_Print(toDisplay);
	}
	vTaskDelete( NULL );
}

void alarmControllerC(void *pvParameter){
	uint8_t monitorId;
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempCO2Level;
	int tempHumidity;
	int tempMotion;
	for( ;; )
	{
		monitorId=xQueueSemaphoreTake(alarmControllerC_Queue, portMAX_DELAY);
		readTheSharedData(monitorId, &tempTempurature, &tempBrightness, &tempHeartbeat, &tempSO2Level, &tempCO2Level, &tempHumidity, &tempMotion);

	}
	vTaskDelete( NULL );
}

void appControllerNC(void *pvParameter){
	uint8_t monitorId;
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempCO2Level;
	int tempHumidity;
	int tempMotion;
	for( ;; )
	{
		monitorId=xQueueSemaphoreTake(appControllerNC_Queue, portMAX_DELAY);
		readTheSharedData(monitorId, &tempTempurature, &tempBrightness, &tempHeartbeat, &tempSO2Level, &tempCO2Level, &tempHumidity, &tempMotion);

	}
	vTaskDelete( NULL );
}
//end of ui level tasks


void app_main(void)
{	
	//init sensors
	i2c_master_init();	    
	max30102_init();
	LCD_Init();
	
	//init interfaceComponents
	
	//init semaphores
	//monitor semaphores
	eTempuratureLock = xSemaphoreCreateMutex();
	iTempuratureLock = xSemaphoreCreateMutex();
	brightnessLock = xSemaphoreCreateMutex();
	heartLock = xSemaphoreCreateMutex();
	CO2LevelLock = xSemaphoreCreateMutex();
	humidityLock = xSemaphoreCreateMutex();
	motionLock = xSemaphoreCreateMutex();
	heartProcessSignal = xSemaphoreCreateBinary();
	
	//ui queues
	ledControllerNC_Queue = xQueueCreateStatic( NUM_MONITORS, sizeof( uint8_t ), &(ledControllerNC_QueueStorage[0]), &ledControllerNC_QueueBuffer);
	ledControllerC_Queue = xQueueCreateStatic( NUM_MONITORS, sizeof( uint8_t ), &(ledControllerC_QueueStorage[0]), &ledControllerC_QueueBuffer);
	lcdControllerNC_Queue = xQueueCreateStatic( NUM_MONITORS, sizeof( uint8_t ), &(lcdControllerNC_QueueStorage[0]), &lcdControllerNC_QueueBuffer);
	alarmControllerC_Queue = xQueueCreateStatic( NUM_MONITORS, sizeof( uint8_t ), &(alarmControllerC_QueueStorage[0]), &alarmControllerC_QueueBuffer);
	appControllerNC_Queue = xQueueCreateStatic( NUM_MONITORS, sizeof( uint8_t ), &(appControllerNC_QueueStorage[0]), &appControllerNC_QueueBuffer);
	
	//error semaphores and queue
	errLocationTrackerLock=xSemaphoreCreateMutex();
	errTypeLock=xSemaphoreCreateMutex();
	errFlagSignal = xSemaphoreCreateCounting(NUM_TASKS,0);	
	errFlagSignalMajor = xSemaphoreCreateBinary();
	errToDecrementQueue = xQueueCreateStatic( NUM_TASKS * 3, sizeof( uint8_t ), &(errQueueStorage[0]), &errToDecrementQueueBuffer);
	for (int i = 0; i < NUM_TASKS; i++){
		recoverySignals[i] = xSemaphoreCreateBinary();
	} 
	
	xTaskCreatePinnedToCore(errHandlerC, NULL, 4096, NULL, ERRHANDLER_P, NULL, TASK_CORE_C);	
	
	//monitor tasks
	xTaskCreatePinnedToCore(eTempratureMonitorNC, NULL, 4096, NULL, ETEMPURATURE_P, NULL, TASK_CORE_NC);
	xTaskCreatePinnedToCore(iTempratureMonitorC, NULL, 4096, NULL, ITEMPURATURE_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(brightnessMonitorNC, NULL, 4096, NULL, BRIGHTNESS_P, NULL, TASK_CORE_NC);
	//note we put the heartbeat monitor and processing on the same core, 
	//even though they will use a lot of the cpu time, because preventing them 
	//from running in paralell helps avoid them accessing the same memory locations simultaneously
	xTaskCreatePinnedToCore(heartbeatMonitorC, NULL, 4096, NULL, HEARTBEAT_MONITOR_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(heartbeatProcessorC, NULL, 4096, NULL, HEARTBEAT_PROCESSOR_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(CO2MonitorC, NULL, 4096, NULL, CO2_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(humidityMonitorNC, NULL, 4096, NULL, HUMIDITY_P, NULL, TASK_CORE_NC);	
	xTaskCreatePinnedToCore(heartbeatMonitorC, NULL, 4096, NULL, LCD_P, NULL, TASK_CORE_NC);
	
	//ui tasks
	xTaskCreatePinnedToCore(ledControllerNC, NULL, 4096, NULL, LED_NC_P, &ledControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerC, NULL, 4096, NULL, LED_C_P, &ledControllerC_Handle, TASK_CORE_C);
	xTaskCreatePinnedToCore(lcdControllerNC, NULL, 4096, NULL, LED_C_P, &lcdControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(alarmControllerC, NULL, 4096, NULL, ALARM_P, &alarmControllerC_Handle, TASK_CORE_C);
	xTaskCreatePinnedToCore(appControllerNC, NULL, 4096, NULL, APP_P, &appControllerNC_Handle, TASK_CORE_NC);
		
	while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}

