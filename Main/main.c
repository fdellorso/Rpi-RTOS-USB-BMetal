#include <stdint.h>
#include <string.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rpi-Irq.h"
#include "semphr.h"
#include "uspi.h"
#include "rsta_bt/btsubsystem.h"


void DoProgress(char label[], int step, int total, int x, int y, COLORREF col)
{
	//progress width
	const int pwidth = 72;
	int len = strlen(label);

	//minus label len
	int width = (pwidth - len)*BitFontWth;
	int pos = (step * width) / total;

	int percent = (step * 100) / total;

	GotoXY(x, y);
	printf("%s[", label);

	//fill progress bar with =
	//COLORREF orgPen = SetDCPenColor(0, col);
	COLORREF orgBrush = SetDCBrushColor(0, col);
	Rectangle(0, (len + 1)*BitFontWth, y*BitFontHt + 2, pos + (len + 1)*BitFontWth, (y + 1)*BitFontHt - 4);

	//SetDCPenColor(0, 0);
	SetDCBrushColor(0, 0);
	Rectangle(0, pos+(len + 1)*BitFontWth, y*BitFontHt + 2, (pwidth + 1)*BitFontWth, (y + 1)*BitFontHt - 4);

	//SetDCPenColor(0, orgPen);
	SetDCBrushColor(0, orgBrush);

	GotoXY(pwidth + 1, y);
	printf("] %3i%%\n", percent);
}


static xSemaphoreHandle barSemaphore = { 0 };

void task1(void *pParam) {
	COLORREF col = 0xFFFF0000;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0)) dir = -dir;

		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{

			DoProgress("HARE  ", step, total, 0, 10, col);

			/* We have finished accessing the shared resource.  Release the
			semaphore. */
			xSemaphoreGive(barSemaphore);
		}
		vTaskDelay(10);
	}
}

void task2(void *pParam) {
	COLORREF col = 0xFF0000FF;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0)) dir = -dir;
		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{

			DoProgress("TURTLE", step, total, 0, 12, col);

			/* We have finished accessing the shared resource.  Release the
			semaphore. */
			xSemaphoreGive(barSemaphore);
		}
		vTaskDelay(35);

	}
}


static bool lit = false;
void task3(void *pvParameters)
{
	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;; )
	{
		/* Print out the name of this task AND the number of times ulIdleCycleCount has been incremented. */
		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{
			GotoXY(0, 14);
			printf("Cpu usage: %u%%    FreeRTOS: %s\n", xLoadPercentCPU(), tskKERNEL_VERSION_NUMBER);
			xSemaphoreGive(barSemaphore);
		}
		if (lit) lit = false; else lit = true;							// Flip lit flag
		set_Activity_LED(lit);											// Turn LED on/off as per new flag
		vTaskDelay(configTICK_RATE_HZ);
	}
}

void task4 (void *pParam) {
	char buf[256];
	while (1) 
	{
		if (xSemaphoreTake(barSemaphore, 40) == pdTRUE)
		{
			GotoXY(0, 15);
			printf("[Task]       [State]  [Prio]  [Stack] [Num]\n");
			vTaskList(&buf[0]);
			printf(&buf[0]);
			printf("\n");


			/* We have finished accessing the shared resource.  Release the
			semaphore. */
			xSemaphoreGive(barSemaphore);
		}
		vTaskDelay(10000);
	}
}


xTaskHandle	xHandleBltProc = NULL;
xQueueHandle xQueBltProc = NULL;
// TaskStatus_t xTaskDetails;

#define INQUIRY_SECONDS		20

void prvTask_BluetoothInitialize(void *pParam) {
	int i = 0;

	/* Stop warnings. */
	( void ) pParam;

	TBTSubSystem *m_Bluetooth = (TBTSubSystem *) pvPortMalloc (sizeof(TBTSubSystem));
	BTSubSystem(m_Bluetooth, 0, NULL);

	// printf("\nBluetooth Initialize...\t\t     Started\n");
	// printf("--------------------------------------------\n");

	if(BTSubSystemInitialize(m_Bluetooth)) {
		printf("Bluetooth Initialize...\t\t    Finished");
	}

	printf("Inquiry is running for %u seconds", INQUIRY_SECONDS);

	// TBTInquiryResults *pInquiryResults = BTSubSystemInquiry(m_Bluetooth, INQUIRY_SECONDS);
	// if (pInquiryResults == 0)
	// {
	// 	prvFunc_Print("Inquiry failed");
	// }

	// prvFunc_Print("Inquiry complete, %u device(s) found", BTInquiryResultsGetCount(pInquiryResults));

	// if (BTInquiryResultsGetCount(pInquiryResults) > 0)
	// {
	// 	prvFunc_Print("# BD address        Class  Name");

	// 	for (unsigned nDevice = 0; nDevice < BTInquiryResultsGetCount(pInquiryResults); nDevice++)
	// 	{
	// 		const u8 *pBDAddress = BTInquiryResultsGetBDAddress(pInquiryResults, nDevice);
	// 		assert (pBDAddress != 0);
			
	// 		const u8 *pClassOfDevice = BTInquiryResultsGetClassOfDevice(pInquiryResults, nDevice);
	// 		assert (pClassOfDevice != 0);
			
	// 		prvFunc_Print("%u %02X:%02X:%02X:%02X:%02X:%02X %02X%02X%02X %s",
	// 				nDevice+1,
	// 				(unsigned) pBDAddress[5],
	// 				(unsigned) pBDAddress[4],
	// 				(unsigned) pBDAddress[3],
	// 				(unsigned) pBDAddress[2],
	// 				(unsigned) pBDAddress[1],
	// 				(unsigned) pBDAddress[0],
	// 				(unsigned) pClassOfDevice[2],
	// 				(unsigned) pClassOfDevice[1],
	// 				(unsigned) pClassOfDevice[0],
	// 				BTInquiryResultsGetRemoteName(pInquiryResults, nDevice));
	// 	}
	// }

	// free(pInquiryResults);

	while(1) {
		i++;

		vTaskDelay(100);
	}
}

void prvTask_BluetoothProcess(void *pParam) {
	int i = 0;

	/* Stop warnings. */
	( void ) pParam;
	
	TBTSubSystem *m_pBTSubSystem = NULL;

	if(xQueueReceive(xQueBltProc, &m_pBTSubSystem, portMAX_DELAY) == pdPASS) {
		vQueueDelete(xQueBltProc);
		printf("%cSubSystem...\t\t\t    Received\n", 0x3e);
	}

	while(1) {
		i++;

		BTSubSystemProcess(m_pBTSubSystem);

		// vTaskGetTaskInfo(xHandleBltProc, &xTaskDetails, pdTRUE, eInvalid);
		// prvFunc_Print("BTPROC Stack: %d", xTaskDetails.usStackHighWaterMark);

		taskYIELD();
		// vTaskDelay(100);
	}
}

void main (void)
{
	Init_EmbStdio(WriteText);										// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed
	printf("Task tick rate: %u\n", configTICK_RATE_HZ);

	USPiInitialize();

	xQueBltProc = xQueueCreate(1, sizeof(TBTSubSystem *));

	/* Attempt to create a semaphore. */
	// vSemaphoreCreateBinary(barSemaphore);

	// if (barSemaphore != NULL)
	// {
		/* The semaphore can now be used. Its handle is stored in the
		xSemahore variable.  Calling xSemaphoreTake() on the semaphore here
		will fail until the semaphore has first been given. */
	// 	xSemaphoreGive(barSemaphore);
	// }

	// xTaskCreate(task1, "HARE  ", 2048, NULL, 4, NULL);
	// xTaskCreate(task2, "TURTLE", 2048, NULL, 4, NULL);
	// xTaskCreate(task3, "TIMER ", 2048, NULL, 3, NULL);
	// xTaskCreate(task4, "DETAIL", 2048, NULL, 2, NULL);

	if(xTaskCreate(prvTask_BluetoothInitialize, (const char *) "BluetoothInitialize",
		4 * configMINIMAL_STACK_SIZE, NULL, 0, NULL) == pdPASS) {
		// if(uxTaskPriorityGet(xHandleBltInit) < configMAX_CO_ROUTINE_PRIORITIES)
		// {
		// 	vTaskSuspend(xHandleBltInit);
		// }
	}

	if(xTaskCreate(prvTask_BluetoothProcess, (const char *) "BluetoothProcess",
		4 * configMINIMAL_STACK_SIZE, NULL, 1, &xHandleBltProc) == pdPASS) {
		// if(uxTaskPriorityGet(xHandleBltProc) < configMAX_CO_ROUTINE_PRIORITIES)
		{
			vTaskSuspend(xHandleBltProc);
		}
	}

	vTaskStartScheduler();
	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	while (1) {
	}
}


