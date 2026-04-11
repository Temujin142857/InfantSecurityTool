#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <Wire.h>
#include "Mpu6050.h"
#include "dht11.h"
#include "max30102.h"
#include "ble.h"
#include "mq2.h"
#include "buzzer.h"
#include "led.h"


#define NUM_TASKS 17
#define NUM_MONITORS 8
#define NUM_UI_TASKS 6

#define MAX_DISPLAY_LENGTH 18

typedef enum {
  ETEMPURATURE = 0,
  ITEMPURATURE,
  BRIGHTNESS,
  HEARTBEAT,
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
	UICONTROLLER_C,
	UICONTROLLER_NC,
} TaskIds;

typedef enum {    
  BRIGHTNESS_P=1,    
  HUMIDITY_P,
  LED_NC_P,
  LCD_P, 
  ETEMPURATURE_P,  	
  APP_P,
	UICONTROLLER_NC_P,
  ERRDECREMENTER_P,
	ERRHANDLER_P,
	MOTION_P,
	ITEMPURATURE_P,
	SMOKE_P,
	HEARTBEAT_P,
	LED_C_P,	   
	ALARM_P,
	UICONTROLLER_C_P,
} TaskPrioroties;

const int tasksInPriorityOrder[NUM_TASKS]={UICONTROLLER_C, ALARM, LED_C, HEARTBEAT, SMOKE, ITEMPURATURE, MOTION, ERRHANDLER, ERRDECREMENTER, UICONTROLLER_NC, APP, ETEMPURATURE, LCD, LED_NC, HUMIDITY, BRIGHTNESS};
const uint32_t delayInMillisecondsForMonitorTasks[NUM_MONITORS]={1000, 1000, 1000, 40, 1000, 1000, 100};


//Which core to pin the tasks to
#define TASK_CORE_NC 0
#define TASK_CORE_C 1

//dataDictionary constants
//note floors and ceilings for raw electrical inputs are handled in the driver files
#define EXTERNAL_TEMPURATURE_MAX 22.2
#define EXTERNAL_TEMPURATURE_MIN 20
#define EXTERNAL_TEMPURATURE_CIEL 50
#define EXTERNAL_TEMPUTATURE_FLOOR -40

#define INTERNAL_TEMPURATURE_MAX 37.5
#define INTERNAL_TEMPURATURE_MIN 36
#define INTERNAL_TEMPURATURE_CIEL 50
#define INTERNAL_TEMPURATURE_FLOOR 20


#define RED_CIEL 200000
#define RED_FLOOR 5000
#define IR_CIEL 200000
#define IR_FLOOR 5000
#define HEARTBEAT_MAX 180
#define HEARTBEAT_MIN 80


#define SMOKE_BATCH_SIZE 5
//smoke level max will have to be defined dynamically, based on the ambient reading
#define SMOKE_WARNING_FACTOR 1.2
#define SMOKE_EMERGENCY_FACTOR 1.4
#define SMOKE_FLOOR_MV   100
#define SMOKE_CIEL_MV   3200


#define HUMIDITY_MAX 70
#define HUMIDITY_MIN 30
#define HUMIDITY_FLOOR 10
#define HUMIDITY_CIEL 90

#define MINUTES_NO_MOVEMENT_MAX 60


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
static SemaphoreHandle_t SO2Lock;
static SemaphoreHandle_t heartbeatLock;
static volatile int smokeLevel;
static SemaphoreHandle_t smokeLevelLock;
static int mq2Baseline_mv = 0;
static volatile int humidity;
static SemaphoreHandle_t humidityLock;
//motion will probably be an array?
static volatile int timeSinceLastMotion;
static SemaphoreHandle_t motionLock;

//temp variables to send via bluetooth
static volatile float tempETempurature;
static SemaphoreHandle_t tempETempuratureLock;
static volatile float tempITempurature;
static SemaphoreHandle_t tempITempuratureLock;
static volatile float tempBrightness;
static SemaphoreHandle_t tempBrightnessLock;
static volatile int tempHeartbeat;
static SemaphoreHandle_t tempHeartbeatLock;
static volatile int tempSO2Level;
static SemaphoreHandle_t tempSO2Lock;
static volatile int tempSmokeLevel;
static SemaphoreHandle_t tempSmokeLock;
static volatile int tempHumidity;
static SemaphoreHandle_t tempHumidityLock;
static volatile int tempMotion;
static SemaphoreHandle_t tempMotionLock;

//assigning the ui task handles to be called by the ui controllers
TaskHandle_t ledControllerNC_Handle = NULL;
TaskHandle_t ledControllerC_Handle = NULL;
TaskHandle_t lcdControllerNC_Handle = NULL;
TaskHandle_t alarmControllerC_Handle = NULL;
TaskHandle_t appControllerNC_Handle = NULL;

//Shared memory and queues for communicating ui changes
//Note the lack of mutexes, the ui controller will be the only task ever writing here
//and it has other tools (notification of tasks) to control timing of access
static volatile int lightsToChangC;
static volatile int lightsToChangeNC;
static char toDisplay[MAX_DISPLAY_LENGTH];

static QueueHandle_t appControllerNC_Queue;
StaticQueue_t appControllerNC_QueueBuffer;
//we do 2 * the amount of monitors just to be safe, in case the controller is slow and some monitors double send
uint8_t appControllerNC_QueueStorage[sizeof(uint8_t) * 2 * NUM_MONITORS];



//queues for indicating ui needs to update
static QueueHandle_t UIControllerC_Queue;
StaticQueue_t UIControllerC_QueueBuffer;
//we do 2 * the amount of monitors just to be safe, in case the controller is slow and some monitors double send
uint8_t UIControllerC_QueueStorage[sizeof(uint8_t) * 2 * NUM_MONITORS];

static QueueHandle_t UIControllerNC_Queue;
StaticQueue_t UIControllerNC_QueueBuffer;
//we do 2 * the amount of monitors just to be safe, in case the controller is slow and some monitors double send
uint8_t UIControllerNC_QueueStorage[sizeof(uint8_t) * 2 * NUM_MONITORS];

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
				Serial.printf("minor error: %i %%\n", i);
				xQueueSend(errToDecrementQueue, ( void * ) &tasksInPriorityOrder[i], 0);
				xTaskCreatePinnedToCore(errDecrementerC, NULL, 4096, NULL, ERRDECREMENTER_P, NULL, TASK_CORE_C);
				
				if(errLocationTracker[tasksInPriorityOrder[i]]>=3){
					//enter error state when it fails three times within a certain time frame
					//revisit when logic for display and lights are more clear					
					Serial.printf("major error: %i %%\n", i);
					//ble_set_all_sensors('U!', 0, 0);
				} else{
					xSemaphoreGive(recoverySignals[tasksInPriorityOrder[i]]);
				}				
			}
		}	
		taskYIELD();
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

void mq2Calibrate(void)
{
    int sum = 0;
    for (int i = 0; i < 50; i++) {
        sum += read_smoke_raw();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    mq2Baseline_mv = sum / 50;
}


//Monitoring tasks
void eTempratureAndHumidityMonitorNC( void *pvParameters )
{	
		float tempHumidity;
		float tempTempurature=0;
		float eTempSum;
    for( ;; )
    {
		uint8_t errCount=0;
		eTempSum=0;
		
		for(uint8_t i=0;i<3;i++){			
			readHumidity(&tempHumidity);
    	readTemperature(&tempTempurature);
			Serial.printf("eTrmputature early: %f %\n", tempTempurature);	
			Serial.printf("humidity early: %f %\n", tempHumidity);	
			if(tempTempurature<EXTERNAL_TEMPUTATURE_FLOOR||tempTempurature>EXTERNAL_TEMPURATURE_CIEL){				
				if(errCount>=3){
					xSemaphoreGive(errFlagSignal);
					errType[ETEMPURATURE]=1;
					waitRecoveryNC(ETEMPURATURE);
					break;
				}	
				errCount++;
				vTaskDelay(pdMS_TO_TICKS(10));
			}
			eTempSum+=tempTempurature;
		}		
		
		xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
		if(errCount<=3){

		tempTempurature=eTempSum/(3-errCount);		
		}
		else {tempTempurature=eTempurature;}
		if(tempTempurature>EXTERNAL_TEMPURATURE_MAX){
		} else if (tempTempurature<EXTERNAL_TEMPURATURE_MIN){
		}
		
		Serial.printf("eTrmputature earlyish: %f %\n", tempTempurature);	
		Serial.printf("humidity earlyish: %f %\n", tempHumidity);	
		eTempurature=tempTempurature;
		xSemaphoreGive(eTempuratureLock);
		int id=ETEMPURATURE;
		xQueueSend(UIControllerNC_Queue, &id, 0);

		if(tempHumidity>HUMIDITY_CIEL||tempHumidity<HUMIDITY_FLOOR){
			errLocationTracker[HUMIDITY]=1;
			xSemaphoreGive(errFlagSignal);
		} else{
			xSemaphoreTake(humidityLock, portMAX_DELAY);
			humidity=tempHumidity;
			xSemaphoreGive(humidityLock);
			int id=HUMIDITY;
			xQueueSend(UIControllerNC_Queue, &id, 0);
		}		

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
			tempTempurature+=t;
			vTaskDelay(pdMS_TO_TICKS(10));
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
		int id=ITEMPURATURE;
		xQueueSend(UIControllerC_Queue, &id, 0);
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

		//xQueueSend(UIControllerNC_Queue, (void *) BRIGHTNESS, 0);
		vTaskDelay(delayInMillisecondsForMonitorTasks[BRIGHTNESS]);
    }
    vTaskDelete( NULL );
}


void heartbeatMonitorC(void *pvParameters)
{
	TickType_t lastWakeTime = xTaskGetTickCount();
	uint8_t awaitingErrProcessing=0;
  int32_t spo2;
  int8_t spo2_valid;
  int32_t heart_rate;
  int8_t hr_valid;
	uint8_t errCount=0;
	uint8_t batchCount=0;
	uint8_t badBatchCount=0;

  for ( ;; ){
		if(awaitingErrProcessing){
			awaitingErrProcessing = waitRecoveryC(HEARTBEAT);
		}
  
		readHeartRateAndBloodOxygen(&spo2, &spo2_valid, &heart_rate, &hr_valid);
		Serial.printf("Heart Rate early: %li bpm\n", (long) heart_rate);
		Serial.printf("SpO2 early: %f %%\n", spo2);			

  	if (hr_valid){			
			heartbeat=heart_rate;
			xSemaphoreGive(heartbeatLock);
  	} 
  	if (spo2_valid){ 	   	
			SO2Level=spo2;
			xSemaphoreGive(SO2Lock);
 	 }   			
		if(!spo2_valid||!hr_valid){		
			errLocationTracker[HEARTBEAT]=1;
			xSemaphoreGive(errFlagSignal);
			awaitingErrProcessing=1;
		}				
		if(spo2_valid||hr_valid){
			int id = HEARTBEAT;
			xQueueSend(UIControllerC_Queue, &id, 0);
		}			
		//this task requires extreme timing precision to properly calculate bpm, so we use delay until instead of delay
		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(delayInMillisecondsForMonitorTasks[HEARTBEAT]));
	}
	vTaskDelete( NULL );
}

void smokeMonitorC( void *pvParameters )
{
	uint8_t awaitingErrProcessing=0;
	uint8_t errCount;
	int tempSmokeLevel_mv=0;
	int sum;
	
	//delay for the monitor to warm up
	vTaskDelay(pdMS_TO_TICKS(60));
	mq2Calibrate();
	
    for( ;; )
    {
		if(awaitingErrProcessing){
			awaitingErrProcessing = waitRecoveryC(HEARTBEAT);
		}
		errCount=0;
		sum=0;
		for (int i=0; i<SMOKE_BATCH_SIZE; i++) {			
			tempSmokeLevel_mv=read_smoke_raw();
			Serial.printf("temp smoke level early in mv: %i", tempSmokeLevel_mv);
			if(tempSmokeLevel_mv >= SMOKE_CIEL_MV && tempSmokeLevel_mv <= SMOKE_FLOOR_MV){
				errCount++;
			} else{
				sum+=tempSmokeLevel_mv;
			}
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		if(errCount>SMOKE_BATCH_SIZE/2){
			errLocationTracker[SMOKE]=1;
			xSemaphoreGive(errFlagSignal);
			awaitingErrProcessing=1;
		}else{
			tempSmokeLevel_mv=sum/(SMOKE_BATCH_SIZE-errCount);			
			xSemaphoreTake(smokeLevelLock, portMAX_DELAY);
			smokeLevel=tempSmokeLevel_mv;
			xSemaphoreGive(smokeLevelLock);
		}
		int id=SMOKE;
		xQueueSend(UIControllerC_Queue, &id, 0);
		vTaskDelay(delayInMillisecondsForMonitorTasks[SMOKE]);
    }
    vTaskDelete( NULL );
}

/*
void humidityMonitorNC( void *pvParameters )
{
	 	
    for( ;; )
    {
		readHumidity(&tempHumidity);
		Serial.printf("Humidity early: %d\n", humidity);		
		
	
		
		vTaskDelay(delayInMillisecondsForMonitorTasks[HUMIDITY]);
    }
    vTaskDelete( NULL );
}

*/

void motionMonitorC( void *pvParameters )
{
	float tempMotion;
	TickType_t lastMovementTick = xTaskGetTickCount();
    for( ;; )
    {
		xSemaphoreTake(I2CLock, portMAX_DELAY);
		readMotion(&tempMotion);
		xSemaphoreGive(I2CLock);	
		Serial.printf("motion early: %f %%\n", tempMotion);			
		//we will have to calibrate
		const float MOTION_THRESHOLD = 5;
		
		if (tempMotion > MOTION_THRESHOLD){
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
		int id=MOTION;
		xQueueSend(UIControllerC_Queue, &id, 0);
		
		vTaskDelay(delayInMillisecondsForMonitorTasks[MOTION]);
    }
    vTaskDelete( NULL );
}

//end of monitor tasks

//ui helpers
//0 is fine, 1 is warning, 2 is emergency
uint8_t eTempuratureCheck(float tempurature){
    if (tempurature < INTERNAL_TEMPURATURE_MIN || tempurature > INTERNAL_TEMPURATURE_MAX) {
        return 2;
    }
    else if (tempurature < 20.0f || tempurature > 24.0f) {
        return 1;
    }
    return 0;
}


uint8_t iTempuratureCheck(float tempurature){
    if (tempurature < EXTERNAL_TEMPURATURE_MIN || tempurature > EXTERNAL_TEMPURATURE_MAX) {
        return 2;
    }
    else if (tempurature < 36.5f || tempurature > 37.5f) {
        return 1;
    }
    return 0;
}

uint8_t heartbeatCheck(int bpm){
    if (bpm < HEARTBEAT_MIN || bpm > HEARTBEAT_MAX) {
        return 2;
    }
    else if (bpm < 100 || bpm > 160) {
        return 1;
    }
    return 0;
}

uint8_t smokeCheck(int smoke){
    if (smoke > SMOKE_EMERGENCY_FACTOR * mq2Baseline_mv) {
        return 2;
    }
    else if (smoke > SMOKE_WARNING_FACTOR * mq2Baseline_mv) {
        return 1;
    }
    return 0;
}

uint8_t humidityCheck(int humidity){
    if (humidity < HUMIDITY_MIN || humidity > HUMIDITY_MAX) {
        return 2;
    }
    else if (humidity < 40 || humidity > 60) {
        return 1;
    }
    return 0;
}

uint8_t motionCheck(int minutesNoMovement){
    if (minutesNoMovement > MINUTES_NO_MOVEMENT_MAX) {
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
	
	char toDisplay[MAX_DISPLAY_LENGTH];
	
	int id;
	
	for(;;){
		xQueueReceive(UIControllerC_Queue, &id, portMAX_DELAY);
		switch (id){						
			case ITEMPURATURE:
				xSemaphoreTake(iTempuratureLock, portMAX_DELAY);
				tempITempurature=iTempurature;
				Serial.printf("itempurature: %f \n", tempITempurature);
				xSemaphoreGive(iTempuratureLock);
				if(iTempuratureCheck(tempITempurature)){
									
				}				
				break;	
						
			case HEARTBEAT:
				xSemaphoreTake(heartbeatLock, portMAX_DELAY);
				tempHeartbeat=heartbeat;				
				xSemaphoreGive(heartbeatLock);
				xSemaphoreTake(SO2Lock, portMAX_DELAY);
				tempSO2Level=SO2Level;
				xSemaphoreGive(SO2Lock);
				Serial.printf("heartbeat: %i \n", tempHeartbeat);
				Serial.printf("SO2: %i \n", tempSO2Level);
				break;	
										
			case SMOKE:
				xSemaphoreTake(smokeLevelLock, portMAX_DELAY);
				tempSmokeLevel=smokeLevel;
				xSemaphoreGive(smokeLevelLock);
				Serial.printf("smoke level: %i \n", tempSmokeLevel);
				break;	
										
			case MOTION:
				xSemaphoreTake(motionLock, portMAX_DELAY);
				tempMotion=timeSinceLastMotion;
				xSemaphoreGive(motionLock);
				Serial.printf("time since last motion: %i \n", tempMotion);
				break;														
		}
		xTaskNotifyGive(appControllerNC_Handle);
		xQueueSend(appControllerNC_Queue, &id, 0);
		taskYIELD();
	}
}

void UIControllerNC(void *pvParameter){
	
	char toDisplay[MAX_DISPLAY_LENGTH];
	
	int id;
	
	for(;;){
		xQueueReceive(UIControllerNC_Queue, &id, portMAX_DELAY);
		switch (id){
			case ETEMPURATURE:
				xSemaphoreTake(eTempuratureLock, portMAX_DELAY);
				tempETempurature=eTempurature;
				xSemaphoreGive(eTempuratureLock);
				//make an array of constants for each state the lights should be in
				//and the message that should be displayed
				//and for what should be sent to the app
				Serial.printf("etempurature: %f \n", tempETempurature);
				if(eTempuratureCheck(tempETempurature)==0){
					//xTaskNotifyGive(ledControllerC_Handle);
				}
				break;
									
			case BRIGHTNESS:
				xSemaphoreTake(brightnessLock, portMAX_DELAY);
				tempBrightness=brightness;
				xSemaphoreGive(brightnessLock);
				Serial.printf("brightness: %f \n", tempBrightness);
				break;	
										
			case HUMIDITY:
				xSemaphoreTake(humidityLock, portMAX_DELAY);
				tempHumidity=humidity;
				xSemaphoreGive(humidityLock);
				Serial.printf("humidity: %i \n", tempHumidity);
				break;												
		}
		xTaskNotifyGive(appControllerNC_Handle);
		xQueueSend(appControllerNC_Queue, &id, 0);
		taskYIELD();
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
		//gpio_toggle(LED_ORANGE);
		taskYIELD();
	}
	vTaskDelete( NULL );
}

void ledControllerC(void *pvParameter){
	uint8_t monitorId;
	int lightsStates;
	int lightsToswitch;
	for( ;; )
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//gpio_toggle(LED_RED);
		taskYIELD();
	}
	vTaskDelete( NULL );
}

/*
void lcdControllerNC(void *pvParameter){
	uint8_t monitorId;
	
	for( ;; )
	{
		//toDisplay=xQueueSemaphoreTake(lcdControllerNC_Queue, portMAX_DELAY);		
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		LCD_Clear();
		LCD_SetCursor(0,0);
		xSemaphoreTake(toDisplayLock, portMAX_DELAY);
		LCD_Print(toDisplay);
		xSemaphoreGive(toDisplayLock);
	}
	vTaskDelete( NULL );
}
*/

void alarmControllerC(void *pvParameter){
	uint8_t toggle;
	
	for( ;; )
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//toggle=xQueueSemaphoreTake(alarmControllerC_Queue, portMAX_DELAY);
		taskYIELD();
	}
	vTaskDelete( NULL );
}

void appControllerNC(void *pvParameter){
	uint8_t monitorId;
	for( ;; )
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		xQueueReceive(appControllerNC_Queue, &monitorId, portMAX_DELAY);	
		switch (monitorId){
			case ETEMPURATURE:
				Serial.printf("sending to ble etempurature: %f \n", tempETempurature);
				ble_set_all_sensors('E', tempETempurature, 0);
				break;
						
			case ITEMPURATURE:
				Serial.printf("sending to ble itempurature: %f \n", tempITempurature);
				ble_set_all_sensors('I', tempITempurature, 0);
				break;	
						
			case BRIGHTNESS:
				Serial.printf("sending to ble brightness: %f \n", tempBrightness);
				ble_set_all_sensors('B', 0, tempBrightness);
				break;	
						
			case HEARTBEAT:
				Serial.printf("sending to ble heartbeat: %i \n", tempHeartbeat);
				Serial.printf("sending to ble SO2: %i \n", tempSO2Level);
				ble_set_all_sensors('C', 0, tempHeartbeat);
				ble_set_all_sensors('O', 0, tempSO2Level);
				break;	
										
			case SMOKE:
				Serial.printf("sending to ble smoke level: %i \n", tempSmokeLevel);
				ble_set_all_sensors('S', 0, tempSmokeLevel);
				break;	
						
			case HUMIDITY:		
				Serial.printf("sending to ble humidity: %i \n", tempHumidity);		
				ble_set_all_sensors('H', 0, tempHumidity);
				break;
										
			case MOTION:
				Serial.printf("sending to ble time since last motion: %i \n", tempMotion);
				ble_set_all_sensors('M', 0, tempMotion);
				break;														
		}
		taskYIELD();

	}
	vTaskDelete( NULL );
}
//end of ui level tasks


void setup()
{	
  Serial.begin(115200);
	delay(2000);
  Serial.printf("ESP32 start");
  Wire.begin(21, 22);
	delay(500);

	//init sensors
	I2CLock = xSemaphoreCreateMutex();
	dht_init();
	mpu_init(&Wire);	
	smoke_init();	
	max30102_init(&Wire);	

	//init interfaceComponents
	led_init();
	buzzer_init();
	ble_init("InfantMonitor");
	
	//init semaphores
	//monitor semaphores
	eTempuratureLock = xSemaphoreCreateMutex();
	iTempuratureLock = xSemaphoreCreateMutex();
	brightnessLock = xSemaphoreCreateMutex();
	heartbeatLock = xSemaphoreCreateMutex();
	SO2Lock = xSemaphoreCreateMutex();
	smokeLevelLock = xSemaphoreCreateMutex();
	humidityLock = xSemaphoreCreateMutex();
	motionLock = xSemaphoreCreateMutex();
	
	//ui queues
	UIControllerC_Queue = xQueueCreateStatic( NUM_MONITORS * 2, sizeof( uint8_t ), &(UIControllerC_QueueStorage[0]), &UIControllerC_QueueBuffer);
	UIControllerNC_Queue = xQueueCreateStatic( NUM_MONITORS * 2, sizeof( uint8_t ), &(UIControllerNC_QueueStorage[0]), &UIControllerNC_QueueBuffer);
	//toDisplayLock = xSemaphoreCreateMutex();
	appControllerNC_Queue = xQueueCreateStatic( NUM_MONITORS * 2, sizeof( uint8_t ), &(appControllerNC_QueueStorage[0]), &appControllerNC_QueueBuffer);
	
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
	
	Serial.printf("tasks start");
	//monitor tasks
	xTaskCreatePinnedToCore(eTempratureAndHumidityMonitorNC, NULL, 4096, NULL, ETEMPURATURE_P, NULL, TASK_CORE_NC);
	Serial.printf("temp and hum started");
	//xTaskCreatePinnedToCore(iTempratureMonitorC, NULL, 8192, NULL, ITEMPURATURE_P, NULL, TASK_CORE_C);
	//xTaskCreatePinnedToCore(brightnessMonitorNC, NULL, 4096, NULL, BRIGHTNESS_P, NULL, TASK_CORE_NC);
	//note we put the heartbeat monitor and processing on the same core, 
	//even though they will use a lot of the cpu time, because preventing them 
	//from running in paralell helps avoid them accessing the same memory locations simultaneously
	xTaskCreatePinnedToCore(heartbeatMonitorC, NULL, 8192, NULL, HEARTBEAT_P, NULL, TASK_CORE_C);
	Serial.printf("heart Started");
	xTaskCreatePinnedToCore(smokeMonitorC, NULL, 8192, NULL, SMOKE_P, NULL, TASK_CORE_C);
	Serial.printf("smoke started");
	//xTaskCreatePinnedToCore(humidityMonitorNC, NULL, 4096, NULL, HUMIDITY_P, NULL, TASK_CORE_NC);	
	xTaskCreatePinnedToCore(motionMonitorC, NULL, 8192, NULL, MOTION_P, NULL, TASK_CORE_C);
	Serial.printf("motion started\n");
	
	//ui tasks
	xTaskCreatePinnedToCore(ledControllerNC, NULL, 4096, NULL, LED_NC_P, &ledControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(ledControllerC, NULL, 4096, NULL, LED_C_P, &ledControllerC_Handle, TASK_CORE_C);
	//xTaskCreatePinnedToCore(lcdControllerNC, NULL, 4096, NULL, LCD_P, &lcdControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(alarmControllerC, NULL, 4096, NULL, ALARM_P, &alarmControllerC_Handle, TASK_CORE_C);
	xTaskCreatePinnedToCore(appControllerNC, NULL, 4096, NULL, APP_P, &appControllerNC_Handle, TASK_CORE_NC);
	xTaskCreatePinnedToCore(UIControllerC, NULL, 8192, NULL, UICONTROLLER_C_P, NULL, TASK_CORE_C);
	xTaskCreatePinnedToCore(UIControllerNC, NULL, 8192, NULL, UICONTROLLER_NC_P, NULL, TASK_CORE_NC);
	Serial.printf("ui started\n");		
}

void loop(){
	checkToReconnect();
}

