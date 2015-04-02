// Simple demo of three threads
// LED blink thread, print thread, and idle loop
#include "FreeRTOS_AVR.h"
#include "nokia.h"
#include "global.h"

// Pins
#define RED_PIN 3
#define GREEN_PIN 9
#define BLUE_PIN 10

// Priorities
#define CPU_M_PRIORITY  4
#define ADC_PRIORITY    3
#define LCD_PRIORITY    2
#define LED_PRIORITY    1

// the counter that the cpu task increments
volatile uint32_t count = 0;
// the counter that the idle task increments
unsigned long ulIdleCycleCount = 0UL;

// Global for LCD to use
float cpu_u = 0;
float voltage = 0;
bool ledstate[3] = {LOW};


//------------------------------------------------------------------------------
// the Utilization Monitor task, highest priority
void Umonitor ( void *pvParameters )
{
	int count=0; // increased every second
	float idleOP = 650085;	// pre-calculated idle operations per second without any other load on the CPU
	float oper = 0, util = 0;
	for( ;; )
	{        
		if(count !=0)	// prevents division by zero (first run)
		{			
			oper = idleOP - (ulIdleCycleCount/count);	// max idle operations in a second - idle operations in a second = NON idle operations in a second
			util = (oper/idleOP)*100;		// percentage of current idle operations opposed to max idle operations is the utilization %
                        if(util<0)
                          util = 0;
			Serial.print(F("CPU Utilization: ")); // print the utilization percentage
                        Serial.println(util);
                        //memset(data, 0, sizeof data);
			if(util > 90.0)
                          Serial.print(F("\n ****** \n WARNING: Utilization factor is higher than 90%%\n ****** \n")); // print a warning if the U% is higher than 90%
			oper = 0;  // pass the utilization value to the global one for the LCD
                        cpu_u = util;
                        util = 0; // reset the U variables
		}
		vTaskDelay( 1000L * (configTICK_RATE_HZ) / 1000L) ; // runs every second
		count++;	// increments the seconds count
	}
}

//------------------------------------------------------------------------------
// LED task
static void vRGBLEDTask(void *pvParameters) {
  
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = (50L * configTICK_RATE_HZ) / 1000L;
  // Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // Switch LED on/off.
    //ledstate = !ledstate; 
    if(voltage<3)
    {
      ledstate[0] = HIGH;
      ledstate[1] = HIGH;
      ledstate[2] = LOW;
    }
    else if(voltage <4)
    {
      ledstate[0] = HIGH;
      ledstate[1] = LOW;
      ledstate[2] = HIGH;
    }
    else
    {
      ledstate[0] = LOW;
      ledstate[1] = HIGH;
      ledstate[2] = HIGH;
    }
    digitalWrite(RED_PIN, ledstate[0]);
    digitalWrite(GREEN_PIN, ledstate[1]);
    digitalWrite(BLUE_PIN, ledstate[2]);
    // Sleep for 200 milliseconds.
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

//------------------------------------------------------------------------------
// LCD task
static void vLCDTask(void *pvParameters) {
  
  char data[20];
  char num[20];
  char* ftosbuff;
  
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = (500L * configTICK_RATE_HZ) / 1000L;
  // Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();
  byte seconds = 0, count = 0;
  unsigned int minutes = 0, hours = 0;

  // Update the LCD every second
  for (;;) {
    LCDclear();
    
    // ADC display
    LCDgotoXY(0,0);
    ftosbuff = floatToString(num, voltage , 2);
    sprintf(data,"A0 = %s V",ftosbuff);
    LCDprint(data);
    
    // LED color display
    LCDgotoXY(0,1);
    if(!ledstate[0])  // common anode RGB LED, negative logic
      LCDprint("LED IS RED");
     else if(!ledstate[1])
       LCDprint("LED IS GREEN");
      else
        LCDprint("LED IS BLUE");
        
     // Runtime display   
     LCDgotoXY(0,2);
     count++;
     seconds = count/2;
     if(seconds>59)
     {
       minutes++;
       count = 0;
       seconds = count/2;
     }
	 if(minutes>59)
	 {
		hours++;
		minutes = 0;
	 }
     if(seconds<=9)
       sprintf(data,"Run: %d:0%d:%d",hours,minutes,seconds);
     else
       sprintf(data,"Run: %d:0%d:%d",hours,minutes,seconds);
     LCDprint(data);
    // Sleep for 0.5 second
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

//------------------------------------------------------------------------------
// ADC task
static void vADC(void *pvParameters) {
  byte sensorPin = A0;    // select the input pin for the potentiometer
  int sensorValue = 0;  // variable to store the value coming from the sensor
  
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = (100L * configTICK_RATE_HZ) / 1000L;
  // Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();
  
  // Acquire ADC values
for (;;) {
    sensorValue = analogRead(sensorPin);
    voltage = sensorValue * (5.0 / 1023.0);
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  LCDinit();
  LCDclear();
  
  // create CPU utilization task
  //xTaskCreate( Umonitor, "CPU Task", 240, NULL, CPU_M_PRIORITY, NULL );
  
   // create ADC task
  xTaskCreate( vADC, "ADC",  configMINIMAL_STACK_SIZE + 50, NULL, ADC_PRIORITY, NULL );
  
  // create LCD task
  xTaskCreate( vLCDTask, "LCD Task", 240, NULL, LCD_PRIORITY, NULL );
  
  // create blink task
  xTaskCreate(vRGBLEDTask, "LED Task", configMINIMAL_STACK_SIZE + 50, NULL, LED_PRIORITY, NULL);


  // start FreeRTOS
  vTaskStartScheduler();

  // should never return
  Serial.println(F("Die"));
  while(1);
}
//------------------------------------------------------------------------------
// WARNING idle loop has a very small stack (configMINIMAL_STACK_SIZE)
// loop must never block
void loop() {
  while(1) {
    // must insure increment is atomic
    // in case of context switch for print
    noInterrupts();
    ulIdleCycleCount++;
    interrupts();
  }
}
