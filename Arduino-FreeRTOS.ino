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
#define ADC_PRIORITY    3
#define LCD_PRIORITY    2
#define LED_PRIORITY    1

// the counter that the idle task increments
unsigned long ulIdleCycleCount = 0UL;

// Global for LCD to use
float voltage = 0;
byte rgbColour[3] = {255, 0, 0}; // the range is 0 - 255
	
// Functions prototypes
void setColourRgb(unsigned int red, unsigned int green, unsigned int blue);

//------------------------------------------------------------------------------
// LED task
static void vRGBLEDTask(void *pvParameters) 
{
	// Task's variables
	int decColour,incColour, i;
  
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = (25L * configTICK_RATE_HZ) / 1000L;
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

  for (;;) 
  {
	// Choose the colors to increment and decrement.
	for (decColour = 0; decColour < 3; decColour += 1)
	{
	   incColour = decColour == 2 ? 0 : decColour + 1;
	  
	  // cross-fade the two colours.
	  for (int i = 0; i < 255; i += 1) 
	  {
		  rgbColour[decColour] -= 1;
		  rgbColour[incColour] += 1;
		  
		  setColourRgb(255 - rgbColour[0],255 - rgbColour[1],255 - rgbColour[2]);
		  
		  // Sleep for 5 milliseconds.
		  vTaskDelayUntil( &xLastWakeTime, xFrequency );
	  }
	}


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
  for (;;) 
  {
    LCDclear();
    
    // ADC display
    LCDgotoXY(0,0);
    ftosbuff = floatToString(num, voltage , 2);
    sprintf(data,"A0 = %s V",ftosbuff);
    LCDprint(data);

    // Runtime display   
    LCDgotoXY(0,1);
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
     sprintf(data,"%dh:%dm:%ds",hours,minutes,seconds);
     LCDprint(data);
	 
	 // LED color display
	 LCDgotoXY(0,2);
	 sprintf(data,"R:%d",rgbColour[0]);
	 LCDprint(data);
	 
	 LCDgotoXY(0,3);
	 sprintf(data,"G:%d",rgbColour[1]);
	 LCDprint(data);
	 
	 LCDgotoXY(0,4);
	 sprintf(data,"B:%d",rgbColour[2]);
	 LCDprint(data);
	 
	 
    // Sleep for 0.5 second
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

//------------------------------------------------------------------------------
// ADC task
static void vADC(void *pvParameters) 
{
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

// Set the value to each of the pins of the RGB LED
void setColourRgb(unsigned int red, unsigned int green, unsigned int blue) 
{
	analogWrite(RED_PIN, red);
	analogWrite(GREEN_PIN, green);
	analogWrite(BLUE_PIN, blue);
}