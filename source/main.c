/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "semphr.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"




/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

/* Tasks periods*/
#define BUTTON1_PERIOD 		50
#define BUTTON2_PERIOD 		50
#define TRANSMITTER_PERIOD 	100
#define RECEIVER_PERIOD 	20
#define LOAD1_PERIOD 		10
#define LOAD2_PERIOD 		100

/* Queue size */
#define QUEUE_SIZE 		10
/* Message string maximum size */
#define QUEUE_MSG_SIZE 	20




/*Tasks prototypes*/
void vButton_1_Monitor_Task( void * pvParameters );
void vButton_2_Monitor_Task( void * pvParameters );
void vPeriodic_Transmitter_Task( void * pvParameters );
void vUart_Receiver_Task(void * pvParameters);
void vLoad_1_Simulation_Task(void * pvParameters);
void vLoad_2_Simulation_Task(void * pvParameters);

/*Hooks prototypes*/
void vApplicationTickHook( void );

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );

/*Tasks Handlers*/
TaskHandle_t xButton_1_Monitor_TaskHandler 		= NULL;
TaskHandle_t xButton_2_Monitor_TaskHandler 		= NULL;
TaskHandle_t xPeriodic_Transmitter_TaskHandler 	= NULL;
TaskHandle_t xUart_Receiver_TaskHandler 		= NULL;
TaskHandle_t xLoad_1_Simulation_TaskHandler 	= NULL;
TaskHandle_t xLoad_2_Simulation_TaskHandler 	= NULL;

QueueHandle_t xMsgs_QueueHandler = NULL;

uint32_t System_totalTime = 0, Idle_totalTime = 0, System_timeIn = 0, Idle_timeIn;
double cpu_load;

typedef struct{
	uint32_t size;
	char msg[QUEUE_MSG_SIZE];
}xQueue_msg;




/*-----------------------------------------------------------*/



/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
 {
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	xMsgs_QueueHandler = xQueueCreate (QUEUE_SIZE,sizeof(xQueue_msg));

	
	xTaskPeriodicCreate(vButton_1_Monitor_Task,					/* Function that implements the task. */
						"Button_1_Monitor",         			/* Text name for the task. */
						100,      								/* Stack size in words, not bytes. */
						( void * ) 0,    						/* Parameter passed into the task. */
						1,										/* Priority at which the task is created. */
						&xButton_1_Monitor_TaskHandler, 		/* Used to pass out the created task's handle. */
						BUTTON1_PERIOD);

	xTaskPeriodicCreate(vButton_2_Monitor_Task,       			/* Function that implements the task. */
						"Button_2_Monitor",          			/* Text name for the task. */
						100,     				 				/* Stack size in words, not bytes. */
						( void * ) 0,    						/* Parameter passed into the task. */
						2,										/* Priority at which the task is created. */
						&xButton_2_Monitor_TaskHandler, 		/* Used to pass out the created task's handle. */
						BUTTON2_PERIOD);									/* Task deadline */
 
	xTaskPeriodicCreate(vPeriodic_Transmitter_Task,       		/* Function that implements the task. */
						"Periodic_Transmitter",          		/* Text name for the task. */
						100,     				 				/* Stack size in words, not bytes. */
						( void * ) 0,    						/* Parameter passed into the task. */
						3,										/* Priority at which the task is created. */
						&xPeriodic_Transmitter_TaskHandler,		/* Used to pass out the created task's handle. */
						TRANSMITTER_PERIOD);									/* Task deadline */
							
 	xTaskPeriodicCreate(vUart_Receiver_Task,       				/* Function that implements the task. */
						"Uart_Receiver",          				/* Text name for the task. */
						100,     				 				/* Stack size in words, not bytes. */
						( void * ) 0,    						/* Parameter passed into the task. */
						4,										/* Priority at which the task is created. */
						&xUart_Receiver_TaskHandler, 			/* Used to pass out the created task's handle. */
						RECEIVER_PERIOD);									/* Task deadline */
 
 	xTaskPeriodicCreate(vLoad_1_Simulation_Task,       			/* Function that implements the task. */
						"Load_1",			          			/* Text name for the task. */
						100,     				 				/* Stack size in words, not bytes. */
						( void * ) 0,    						/* Parameter passed into the task. */
						5,										/* Priority at which the task is created. */
						&xLoad_1_Simulation_TaskHandler,		/* Used to pass out the created task's handle. */
						LOAD1_PERIOD);									/* Task deadline */
 
 	xTaskPeriodicCreate(vLoad_2_Simulation_Task,       			/* Function that implements the task. */
						"Load_2",          						/* Text name for the task. */
						100,     				 				/* Stack size in words, not bytes. */
						( void * ) 0,    						/* Parameter passed into the task. */
						6,										/* Priority at which the task is created. */
						&xLoad_2_Simulation_TaskHandler,		/* Used to pass out the created task's handle. */
						LOAD2_PERIOD);									/* Task deadline */ 
	
	System_timeIn = T1TC;

	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/


/*-----------------Tasks-------------------*/
void vButton_1_Monitor_Task( void * pvParameters )
{
	TickType_t xLastWakeTime;
	pinState_t curr_buttonStatus;
	xQueue_msg rising_msg = {15,"rising button 1"};
	xQueue_msg falling_msg = {16,"falling button 1"};
	pinState_t prev_buttonState = PIN_IS_LOW;
 	const TickType_t xFrequency = BUTTON1_PERIOD;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 3 );

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	for( ;; )
	{
		//TODO✅: read botton 1 and send message to the uart
		curr_buttonStatus = GPIO_read(PORT_1,PIN0);
		
		if( curr_buttonStatus != prev_buttonState)
		{
			if(curr_buttonStatus == PIN_IS_LOW)
			{
				xQueueSend(xMsgs_QueueHandler,(xQueue_msg *)(&rising_msg),0);
			}else
			{
				xQueueSend(xMsgs_QueueHandler,(xQueue_msg *)(&falling_msg),0);
			}
			prev_buttonState = curr_buttonStatus;
		}
		
		
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		
	}
}

void vButton_2_Monitor_Task( void * pvParameters )
{
	TickType_t xLastWakeTime;
	pinState_t curr_buttonStatus;
	xQueue_msg rising_msg = {15,"rising button 2"};
	xQueue_msg falling_msg = {16,"falling button 2"};
	pinState_t prev_buttonState = PIN_IS_LOW;
 	const TickType_t xFrequency = BUTTON2_PERIOD;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 4 );

	// Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	for( ;; )
	{
		//TODO✅: read botton 2 and send message to the uart
		curr_buttonStatus = GPIO_read(PORT_1,PIN1);
		
		if( curr_buttonStatus != prev_buttonState)
		{
			if(curr_buttonStatus == PIN_IS_LOW)
			{
				xQueueSend(xMsgs_QueueHandler,(xQueue_msg *)(&rising_msg),0);
			}else
			{
				xQueueSend(xMsgs_QueueHandler,(xQueue_msg *)(&falling_msg),0);
			}
			prev_buttonState = curr_buttonStatus;
		}
		
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		
	}
}

void vPeriodic_Transmitter_Task( void * pvParameters )
{
	TickType_t xLastWakeTime;
	xQueue_msg msg = {12,"Periodic msg"};
 	const TickType_t xFrequency = TRANSMITTER_PERIOD;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 5 );

	// Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	
	for( ;; )
	{
		//TODO✅: send message to the uart
		
		xQueueSend(xMsgs_QueueHandler,(xQueue_msg *)(&msg),0);
		// vSerialPutString("Periodic msg\n",12);

		vTaskDelayUntil( &xLastWakeTime, xFrequency );

		
	}
}

void vUart_Receiver_Task(void * pvParameters)
{
	TickType_t xLastWakeTime;
 	const TickType_t xFrequency = RECEIVER_PERIOD;
	xQueue_msg msg_buffer;
	vTaskSetApplicationTaskTag( NULL, ( void * ) 6 );

	// Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	for(;;)
  	{
		//TODO✅: Send messages received form tasks to the UART
		if(xQueueReceive(xMsgs_QueueHandler,&msg_buffer,0) == pdPASS)
		{
			xSerialPutChar('\n');
			vSerialPutString(msg_buffer.msg,msg_buffer.size);

		}
		
		
		vTaskDelayUntil( &xLastWakeTime, xFrequency );

		
			
 	}
}


void vLoad_1_Simulation_Task(void * pvParameters)
{
	TickType_t xLastWakeTime;
 	const TickType_t xFrequency = LOAD1_PERIOD;
	/* This task is going to be represented by a voltage scale of 1. */
    vTaskSetApplicationTaskTag( NULL, ( void * ) 1 );

	// Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	
	for( ;; )
	{
		//TODO✅: set the execution time to 5ms
		int i;
		for( i = 0; i < 33200; i++ )
		{
			i = i;
		}

		
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		
		
	}
}
void vLoad_2_Simulation_Task(void * pvParameters)
{
	TickType_t xLastWakeTime;
 	const TickType_t xFrequency = LOAD2_PERIOD;
	/* This task is going to be represented by a voltage scale of 2. */
    vTaskSetApplicationTaskTag( NULL, ( void * ) 2 );

	// Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
	
	for( ;; )
	{
		//TODO✅: set the execution time to 12ms
		int i;
		for(i = 0; i < 80000; i++ )
		{
			i = i;
		}
		
		
		
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		
	}

}
/*------------------------End of Tasks-----------------------*/

/*-----------------Hooks-----------------*/
void vApplicationTickHook( void )
{
	/*Write your code here*/
	GPIO_write(PORT_0, PIN0, PIN_IS_HIGH);
	GPIO_write(PORT_0, PIN0, PIN_IS_LOW);
	
}

/*--------------End of Hooks--------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}

/*-----------------------------------------------------------*/


