//
// btsubsystem.c
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <rsta_bt/btsubsystem.h>
// #include <rsta_bt/bttask.h>

#include <uspi/devicenameservice.h>
#include <uspi/util.h>
#include <uspi/assert.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <stufa_Task.h>

extern xTaskHandle	xHandleBltProc;
extern xQueueHandle	xQueBltProc;

extern xTaskHandle	xHandleBltInit;
extern TaskStatus_t xTaskDetails;

void BTSubSystem (TBTSubSystem *pThis, u32 nClassOfDevice, const char *pLocalName)
{
	pThis->m_pUARTTransport = 0;
	pThis->m_pHCILayer = (TBTHCILayer *) malloc (sizeof(TBTHCILayer));
	pThis->m_pLogicalLayer = (TBTLogicalLayer *) malloc (sizeof(TBTLogicalLayer));
	if(nClassOfDevice != 0 && pLocalName != NULL) {
		BTHCILayer(pThis->m_pHCILayer, nClassOfDevice, pLocalName);
	}
	else {
		BTHCILayer(pThis->m_pHCILayer, BT_CLASS_DESKTOP_COMPUTER, "Raspberry Pi");
	}
	BTLogicalLayer(pThis->m_pLogicalLayer, pThis->m_pHCILayer);
}

void _BTSubSystem (TBTSubSystem *pThis)
{
	free (pThis->m_pUARTTransport);
	pThis->m_pUARTTransport = 0;
}

boolean BTSubSystemInitialize (TBTSubSystem *pThis)
{
	// if USB transport not available, UART still free and this is a RPi 3B or Zero W:
	//	use UART transport
	if (DeviceNameServiceGetDevice (DeviceNameServiceGet (), "ubt1", FALSE) == 0)
	{
		assert (pThis->m_pUARTTransport == 0);
		pThis->m_pUARTTransport = (TBTUARTTransport *) malloc (sizeof(TBTUARTTransport));
		BTUARTTransport(pThis->m_pUARTTransport);
		assert (pThis->m_pUARTTransport != 0);

		if (!BTUARTTransportInitialize(pThis->m_pUARTTransport, 0))
		{
			return FALSE;
		}
	}

	if (!BTHCILayerInitialize(pThis->m_pHCILayer))
	{
		return FALSE;
	}
	
	if (!BTLogicalLayerInitialize(pThis->m_pLogicalLayer))
	{
		return FALSE;
	}

	if(xQueueSend(xQueBltProc, &pThis, 0) == pdPASS) {
		prvFunc_Print("%cSubSystem...\t\t\t      Sended", 0x3e);
		vTaskResume(xHandleBltProc);
	}

	while (!BTDeviceManagerDeviceIsRunning(BTHCILayerGetDeviceManager(pThis->m_pHCILayer)))
	{
		vTaskGetTaskInfo(xHandleBltInit, &xTaskDetails, pdTRUE, eInvalid);
		prvFunc_Print("BTINIT Stack: %d", xTaskDetails.usStackHighWaterMark);

		taskYIELD();
	}

	return TRUE;
}

void BTSubSystemProcess (TBTSubSystem *pThis)
{
	BTHCILayerProcess(pThis->m_pHCILayer);

	BTLogicalLayerProcess(pThis->m_pLogicalLayer);
}

TBTInquiryResults *BTSubSystemInquiry (TBTSubSystem *pThis, unsigned nSeconds)
{
	return BTLogicalLayerInquiry(pThis->m_pLogicalLayer, nSeconds);
}
