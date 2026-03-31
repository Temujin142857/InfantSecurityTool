#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
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
#include "MQ2.h"
#include "MPU6050.h"


#define NUM_TASKS 16
#define NUM_MONITORS 8
#define NUM_UI_TASKS 6

#define MAX_DISPLAY_LENGTH 18

typedef enum {
    ETEMPURATURE = 0,
    ITEMPURATURE,
    BRIGHTNESS,
    HEARTBEAT_MONITOR,
	HEARTBEAT_PROCESSOR,
    SMOKE,
    HUMIDITY,
	MOTION,
    LED_NC,
    LED_C,
    LCD,
    ALARM,
    APP,
	ERRHANDLER,
	ERRDECREMENTER,
	UICONTROLLER
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
	SMOKE_P,
	HEARTBEAT_MONITOR_P,
	HEARTBEAT_PROCESSOR_P,	
	LED_C_P,	   
	ALARM_P,
	UICONTROLLER_P
} TaskPrioroties;

const int tasksInPriorityOrder[NUM_TASKS]={UICONTROLLER, ALARM, LED_C, HEARTBEAT_PROCESSOR, HEARTBEAT_MONITOR, SMOKE, ITEMPURATURE, MOTION, ERRHANDLER, ERRDECREMENTER, APP, ETEMPURATURE, LCD, LED_NC, HUMIDITY, BRIGHTNESS};
const uint32_t delayInMillisecondsForMonitorTasks[NUM_MONITORS]={20000, 15000, 30000, 40, 1000, 30000, 300, 100};


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

#define SMOKE_BATCH_SIZE 5
#define SMOKE_LEVEL_MAX 1
#define SMOKE_FLOOR_MV   100
#define SMOKE_CIEL_MV   3200

//not sure what these will look like yet


#define HUMIDITY_MAX 0.55
#define HEARTBEAT_MIN 1
#define HEARTBEAT_MAX 1

static SemaphoreHandle_t I2CLock;

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
static volatile int smokeLevel;
static SemaphoreHandle_t smokeLevelLock;
static int mq2Baseline_mv = 0;
static volatile int humidity;
static SemaphoreHandle_t humidityLock;
//motion will probably be an array?
static volatile int timeSinceLastMotion;
static SemaphoreHandle_t motionLock;



//assigning the ui task handles in case I want to use notification later
TaskHandle_t ledControllerNC_Handle = NULL;
TaskHandle_t ledControllerC_Handle = NULL;
TaskHandle_t lcdControllerNC_Handle = NULL;
TaskHandle_t alarmControllerC_Handle = NULL;
TaskHandle_t appControllerNC_Handle = NULL;

//shared memory for communicating ui changes
static volatile int lightsToChangC;
static volatile int lightsToChangeNC;
static volatile char toDisplay[MAX_DISPLAY_LENGTH];

//queue for indicating ui needs to update
static QueueHandle_t UIControllerC_Queue;
StaticQueue_t UIController_QueueBuffer;
//we do 2 * the amount of monitors just to be safe, in case the controller is slow and some monitors double send
uint8_t UIController_QueueStorage[sizeof(uint8_t) * 2 * NUM_MONITORS];

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
static volatile uint8_t errWatingTracker[NUM_TASKS]={0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0};

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
	//xQueueSend(ledControllerNC_Queue, (void *) &id, 0);
	//xQueueSend(ledControllerC_Queue, (void *) &id, 0);
	//xQueueSend(lcdControllerNC_Queue, (void *) &id, 0);
	//xQueueSend(alarmControllerC_Queue, (void *) &id, 0);
	//xQueueSend(appControllerNC_Queue, (void *) &id, 0);
}


void mq2Calibrate(void)
{
	
    int sum = 0;
    for (int i = 0; i < 50; i++) {
        sum += mq2_read_mv();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    mq2Baseline_mv = sum / 50;
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
		xSemaphoreTake(I2CLock, portMAX_DELAY);
		samplesInBatch = max30102_read_fifo(&tempSamples[0], maxSamplesPerReadBatch);
		xSemaphoreGive(I2CLock);
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
        if (xSemaphoreTake(heartProcessSignal, portMAX_DELAY)){
            // Call Maxim algorithm 
			rf_heart_rate_and_oxygen_saturation(activeIRSamples, HEARTBEAT_BUFFER_SIZE, activeREDSamples, &spo2, &spo2_valid, &heart_rate, &hr_valid, &ratio, &correl);

			if(spo2_valid||hr_valid){
				xSemaphoreTake(heartLock, portMAX_DELAY);
			}							
            if (hr_valid){
				printf("Heart Rate: %li bpm\n", (long) heart_rate);
				heartbeat=heart_rate;
            } 
            if (spo2_valid){
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

void smokeMonitorC( void *pvParameters )
{
	uint8_t awaitingErrProcessing=0;
	uint8_t errCount;
	int tempSmokeLevel_mv;
	int sum;
	
	vTaskDelay(pdMS_TO_TICKS(60));
	mq2Calibrate();
	
    for( ;; )
    {
		if(awaitingErrProcessing){
			awaitingErrProcessing = waitRecoveryC(HEARTBEAT_MONITOR);
		}
		errCount=0;
		sum=0;
		for (int i=0; i<SMOKE_BATCH_SIZE; i++) {			
			tempSmokeLevel_mv=mq2_read_mv();
			if(tempSmokeLevel_mv >= SMOKE_CIEL_MV && tempSmokeLevel_mv <= SMOKE_FLOOR_MV){
				errCount++;
			} else{
				sum+=tempSmokeLevel_mv;
			}
		}
		if(errCount>SMOKE_BATCH_SIZE/2){
			errLocationTracker[SMOKE]=1;
			xSemaphoreGive(errFlagSignal);
			awaitingErrProcessing=1;
		}else{
			tempSmokeLevel_mv=sum/SMOKE_BATCH_SIZE;			
			xSemaphoreTake(smokeLevelLock, portMAX_DELAY);
			smokeLevel=tempSmokeLevel_mv;
			xSemaphoreGive(smokeLevelLock);
		}
		
		notifyAllUITasks(SMOKE);
		vTaskDelay(delayInMillisecondsForMonitorTasks[SMOKE]);
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

void motionMonitorNC( void *pvParameters )
{
	mpu6050_data_t data;
	TickType_t lastMovementTick = xTaskGetTickCount();
    for( ;; )
    {
		xSemaphoreTake(I2CLock, portMAX_DELAY);
		mpu6050_read(&data);
		xSemaphoreGive(I2CLock);	
		
		float delta = motion_delta(&data);
		//we will have to calibrate
		const float MOTION_THRESHOLD = 0.15f;
		
		if (delta > MOTION_THRESHOLD){
			// Movement detected → reset timer
			lastMovementTick = xTaskGetTickCount();
		}		
		TickType_t now = xTaskGetTickCount();
		TickType_t diffTicks = now - lastMovementTick;
		int seconds = diffTicks * portTICK_PERIOD_MS / 1000;
		int minutes = seconds / 60;
		
		xSemaphoreTake(motionLock, portMAX_DELAY);
		timeSinceLastMotion = minutes;
		xSemaphoreGive(motionLock);
		notifyAllUITasks(MOTION);
		
		vTaskDelay(delayInMillisecondsForMonitorTasks[HUMIDITY]);
    }
    vTaskDelete( NULL );
}

//end of monitor tasks

//ui helpers
//0 is fine, 1 is warning, 2 is emergency
uint8_t eTempuratureCheck(float tempurature){
    if (tempurature < 16.0f || tempurature > 28.0f) {
        return 2;
    }
    else if (tempurature < 20.0f || tempurature > 24.0f) {
        return 1;
    }
    return 0;
}


uint8_t iTempuratureCheck(float tempurature){
    if (tempurature < 35.5f || tempurature > 38.5f) {
        return 2;
    }
    else if (tempurature < 36.5f || tempurature > 37.5f) {
        return 1;
    }
    return 0;
}

uint8_t heartbeatCheck(int bpm){
    if (bpm < 80 || bpm > 180) {
        return 2;
    }
    else if (bpm < 100 || bpm > 160) {
        return 1;
    }
    return 0;
}

uint8_t smokeCheck(int smoke){
    if (smoke > 2000) {
        return 2;
    }
    else if (smoke > 1000) {
        return 1;
    }
    return 0;
}

uint8_t humidityCheck(int humidity){
    if (humidity < 30 || humidity > 70) {
        return 2;
    }
    else if (humidity < 40 || humidity > 60) {
        return 1;
    }
    return 0;
}

uint8_t motionCheck(int minutesNoMovement){
    if (minutesNoMovement > 60) {
        return 2;
    }
    else if (minutesNoMovement > 20) {
        return 1;
    }
    return 0;
}




//ui level tasks
//maybe make this the controller, and have it call the other ui tasks
//like it processes the data, and decides who needs to update their values
void UIControllerC(void *pvParameter){
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempSmokeLevel;
	int tempHumidity;
	int tempMotion;
	char toDisplay[MAX_DISPLAY_LENGTH];
	
	int id;
	
	for(;;){
		id=xQueueSemaphoreTake(UIControllerC_Queue, portMAX_DELAY);
		switch (id){
			case ETEMPURATURE:
				xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
				tempTempurature=eTempurature;
				xSemaphoreGive(eTempuratureLock);
				//make an array of constants for each state the lights should be in
				//and the message that should be displayed
				//and for what should be sent to the app
				printf("etempurature: %f \n", tempTempurature);
				if(eTempuratureCheck(tempTempurature)==0){
					//xTaskNotifyGive(ledControllerC_Handle);
				}
				break;
						
			case ITEMPURATURE:
				xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
				tempTempurature=iTempurature;
				printf("itempurature: %f \n", tempTempurature);
				xSemaphoreGive(eTempuratureLock);
				if(iTempuratureCheck(tempTempurature)){
									
				}
				break;	
						
			case BRIGHTNESS:
				xSemaphoreTake(brightnessLock, portMAX_DELAY);
				tempBrightness=brightness;
				xSemaphoreGive(brightnessLock);
				printf("brightness: %f \n", tempBrightness);
				break;	
						
			case HEARTBEAT_PROCESSOR:
				xSemaphoreTake(heartLock, portMAX_DELAY);
				tempHeartbeat=heartbeat;
				tempSO2Level=SO2Level;
				xSemaphoreGive(heartLock);
				printf("heartbeat: %i \n", tempHeartbeat);
				printf("SO2: %i \n", tempSO2Level);
				break;	
										
			case SMOKE:
				xSemaphoreTake(smokeLevelLock, portMAX_DELAY);
				tempSmokeLevel=smokeLevel;
				xSemaphoreGive(smokeLevelLock);
				printf("smoke level: %i \n", tempSmokeLevel);
				break;	
						
			case HUMIDITY:
				xSemaphoreTake(humidityLock, portMAX_DELAY);
				tempHumidity=humidity;
				xSemaphoreGive(humidityLock);
				printf("humidity: %i \n", tempHumidity);
				break;
										
			case MOTION:
				xSemaphoreTake(motionLock, portMAX_DELAY);
				tempMotion=timeSinceLastMotion;
				xSemaphoreGive(motionLock);
				printf("time since last motion: %i \n", tempMotion);
				break;														
		}
	}
}


//this one just takes an int, which represents which lights to switch
void ledControllerNC(void *pvParameter){
	uint8_t monitorId;
	int lightsStates;
	int lightsToswitch;
	
	for( ;; )
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		gpio_toggle(LED_ORANGE);
	}
	vTaskDelete( NULL );
}

void ledControllerC(void *pvParameter){
	uint8_t monitorId;
	int lightsStates;
	int lightsToswitch;
	for( ;; )
	{
		//monitorId=xQueueSemaphoreTake(ledControllerC_Queue, portMAX_DELAY);
		gpio_toggle(LED_RED);
	}
	vTaskDelete( NULL );
}

void lcdControllerNC(void *pvParameter){
	uint8_t monitorId;
	;
	char toDisplay[MAX_DISPLAY_LENGTH];
	for( ;; )
	{
		//toDisplay=xQueueSemaphoreTake(lcdControllerNC_Queue, portMAX_DELAY);		

		LCD_Clear();
		LCD_SetCursor(0,0);
		LCD_Print(toDisplay);
	}
	vTaskDelete( NULL );
}

void alarmControllerC(void *pvParameter){
	uint8_t toggle;
	
	for( ;; )
	{
		//toggle=xQueueSemaphoreTake(alarmControllerC_Queue, portMAX_DELAY);
	}
	vTaskDelete( NULL );
}

void appControllerNC(void *pvParameter){
	uint8_t monitorId;
	float tempTempurature;
	float tempBrightness;
	int tempHeartbeat;
	int tempSO2Level;
	int tempSmokeLevel;
	int tempHumidity;
	int tempMotion;
	for( ;; )
	{
		//monitorId=xQueueSemaphoreTake(appControllerNC_Queue, portMAX_DELAY);	
	}
	vTaskDelete( NULL );
}
//end of ui level tasks


void app_main(void)
{	
	//init sensors
	I2CLock = xSemaphoreCreateMutex();
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
	smokeLevelLock = xSemaphoreCreateMutex();
	humidityLock = xSemaphoreCreateMutex();
	motionLock = xSemaphoreCreateMutex();
	heartProcessSignal = xSemaphoreCreateBinary();
	
	//ui queue
	UIControllerC_Queue = xQueueCreateStatic( NUM_TASKS * 3, sizeof( uint8_t ), &(UIController_QueueStorage[0]), &UIController_QueueBuffer);
	
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
	xTaskCreatePinnedToCore(smokeMonitorC, NULL, 4096, NULL, SMOKE_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(humidityMonitorNC, NULL, 4096, NULL, HUMIDITY_P, NULL, TASK_CORE_NC);	
	xTaskCreatePinnedToCore(motionMonitorNC, NULL, 4096, NULL, MOTION_P, NULL, TASK_CORE_NC);
	
	//ui tasks
	xTaskCreatePinnedToCore(ledControllerNC, NULL, 4096, NULL, LED_NC_P, &ledControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerC, NULL, 4096, NULL, LED_C_P, &ledControllerC_Handle, TASK_CORE_C);
	xTaskCreatePinnedToCore(lcdControllerNC, NULL, 4096, NULL, LCD_P, &lcdControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(alarmControllerC, NULL, 4096, NULL, ALARM_P, &alarmControllerC_Handle, TASK_CORE_C);
	xTaskCreatePinnedToCore(appControllerNC, NULL, 4096, NULL, APP_P, &appControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(UIControllerC, NULL, 4096, NULL, UICONTROLLER_P, NULL, TASK_CORE_C);
		
	while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}

