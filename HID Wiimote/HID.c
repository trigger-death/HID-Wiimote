/*

Copyright (C) 2017 Julian Löhr
All rights reserved.

Filename:
	HID.c

Abstract:
	Contains IOQueues and HID specific functions.
	Handles all HID Requests and translates the Wiimote State into HID reports.
*/
#include "HID.h"

#include "Device.h"
#include "Bluetooth.h"
#include "HIDDescriptors.h"
#include "WiimoteToHIDParser.h"

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL HIDInternalDeviceControlCallback;
EVT_READ_IO_CONTROL_BUFFER_FILL_BUFFER HIDFillReadBufferCallback;
EVT_WDF_IO_QUEUE_IO_WRITE HIDWriteCallback;
EVT_WDF_IO_QUEUE_IO_READ HIDReadCallback;
EVT_WDF_IO_QUEUE_IO_DEFAULT HIDIoDefaultCallback;

VOID ProcessGetDeviceDescriptor(_In_ WDFREQUEST Request);
VOID ProcessGetReportDescriptor(_In_ WDFREQUEST Request);
VOID ProcessGetDeviceAttributes(_In_ WDFREQUEST Request,_In_ PHID_DEVICE_CONTEXT HIDContext);
VOID ProcessGetString(_In_ WDFREQUEST Request, _In_ PDEVICE_CONTEXT DeviceContext);
VOID ForwardReadReportRequest(_In_ WDFREQUEST Request, _In_ PDEVICE_CONTEXT DeviceContext);
VOID ProcessInternalReadReport(_In_ WDFREQUEST Request, _In_ PDEVICE_CONTEXT DeviceContext, _In_ size_t OutputBufferLength);
VOID ProcessAddresses(_In_ WDFREQUEST Request, _In_ PDEVICE_CONTEXT DeviceContext);
VOID ProcessSetOutputReport(_In_ WDFREQUEST Request, _In_ PDEVICE_CONTEXT DeviceContext, _In_ size_t OutputBufferLength);

NTSTATUS HIDPrepare(
	_In_ PDEVICE_CONTEXT DeviceContext
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	Status = GetVendorAndProductID(DeviceContext->IoTarget, &(DeviceContext->HIDContext.VendorID), &(DeviceContext->HIDContext.ProductID));
	if(!NT_SUCCESS(Status))
	{
		return Status;
	}

	return Status;
}

NTSTATUS 
HIDCreateQueues(
	_In_ WDFDEVICE Device, 
	_In_ PDEVICE_CONTEXT DeviceContext
	)
{
	NTSTATUS Status;
	WDF_IO_QUEUE_CONFIG QueueConfig;
	PHID_DEVICE_CONTEXT HIDContext = &DeviceContext->HIDContext;

	// Create Default Queue
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);
	QueueConfig.EvtIoInternalDeviceControl = HIDInternalDeviceControlCallback;
#ifdef PASSTHROUGH
	QueueConfig.EvtIoDeviceControl = HIDInternalDeviceControlCallback;
	QueueConfig.EvtIoWrite = HIDWriteCallback;
	QueueConfig.EvtIoRead = HIDReadCallback;
	//QueueConfig.EvtIoDefault = HIDIoDefaultCallback;
	QueueConfig.DefaultQueue = TRUE;
	QueueConfig.PowerManaged = WdfFalse;
#endif

	Status = WdfIoQueueCreate(Device, &QueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &(HIDContext->DefaultIOQueue));
	if(!NT_SUCCESS(Status))
	{
		TraceStatus("Creating DefaultIOQueue failed", Status);
		return Status;
	}

/*#ifdef PASSTHROUGH
	Status = WdfDeviceConfigureRequestDispatching(Device, HIDContext->DefaultIOQueue, WdfRequestTypeRead);
	if (!NT_SUCCESS(Status))
	{
		TraceStatus("WdfRequestTypeRead", Status);
		return Status;
	}
	Status = WdfDeviceConfigureRequestDispatching(Device, HIDContext->DefaultIOQueue, WdfRequestTypeWrite);
	if (!NT_SUCCESS(Status))
	{
		TraceStatus("WdfRequestTypeWrite", Status);
		return Status;
	}
	Status = WdfDeviceConfigureRequestDispatching(Device, HIDContext->DefaultIOQueue, WdfRequestTypeDeviceControl);
	if (!NT_SUCCESS(Status))
	{
		TraceStatus("WdfRequestTypeDeviceControl", Status);
		return Status;
	}
	Status = WdfDeviceConfigureRequestDispatching(Device, HIDContext->DefaultIOQueue, WdfRequestTypeDeviceControlInternal);
	if (!NT_SUCCESS(Status)) {
		TraceStatus("WdfRequestTypeDeviceControl", Status);
		return Status;
	}
#endif*/

//#ifndef PASSTHROUGH
	// Create Read Buffer Queue
	Status = ReadIoControlBufferCreate(&HIDContext->ReadBuffer, DeviceContext->Device, DeviceContext, HIDFillReadBufferCallback, WIIMOTE_REPORT_SIZE);
	if (!NT_SUCCESS(Status))
	{
		TraceStatus("Creating HID Read Buffer failed", Status);
		return Status;
	}
//#endif

	return Status;
}

PHID_DEVICE_CONTEXT
GetHIDContext(
	_In_ WDFQUEUE Queue
	)
{
	return &(GetDeviceContext(WdfIoQueueGetDevice(Queue))->HIDContext);
}
VOID
HIDIoDefaultCallback(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    )
{
	UNREFERENCED_PARAMETER(Queue);

	WDF_REQUEST_PARAMETERS Parameters;
	WDF_REQUEST_PARAMETERS_INIT(&Parameters);

	WdfRequestGetParameters(Request, &Parameters);
	Trace("HIDIoDefaultCallback %u %u", Parameters.Type, Parameters.MinorFunction);

	WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
HIDWriteCallback(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t Length
	)
{
	Trace("HIDWriteCallback");
	NTSTATUS Status = STATUS_SUCCESS;

	BYTE * Buffer = NULL;
	size_t Length2 = 0;

	Status = WdfRequestRetrieveInputBuffer(Request, 1, &Buffer, &Length2);
	if (!NT_SUCCESS(Status)) {
		TraceStatus("HIDWriteCallback:WdfRequestRetrieveInputBuffer: ", Status);
		WdfRequestComplete(Request, Status);
		return;
	}
	Trace("HIDWriteCallback Data:");
	PrintBytes(Buffer, Length2);
	Trace("HIDWriteCallback Length: %u", Length);

	Status = BluetoothWriteReport(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)), Length);

	WdfRequestCompleteWithInformation(Request, Status, Length);
}

VOID
HIDReadCallback(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Length);

	Trace("HIDReadCallback");
	NTSTATUS Status = STATUS_SUCCESS;

	BYTE * Buffer = NULL;
	size_t Length2 = 0;

	Status = WdfRequestRetrieveInputBuffer(Request, 1, &Buffer, &Length2);
	if (!NT_SUCCESS(Status)) {
		TraceStatus("HIDWriteCallback:WdfRequestRetrieveInputBuffer: ", Status);
		WdfRequestComplete(Request, Status);
		return;
	}
	Trace("HIDWriteCallback Data:");
	PrintBytes(Buffer, Length2);
	Trace("HIDWriteCallback Length: %u", Length);

	Status = BluetoothWriteReport(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)), Length);

	WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
}

VOID
HIDInternalDeviceControlCallback(
	_In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
	)
{
	UNREFERENCED_PARAMETER(Queue);
	//UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	Trace("HIDInternalDeviceControlCallback: %u", IoControlCode);

	switch(IoControlCode)
	{
	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		ProcessGetDeviceDescriptor(Request);
		break;
	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		ProcessGetDeviceAttributes(Request, GetHIDContext(Queue));
		break;
	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		ProcessGetReportDescriptor(Request);
		break;
	case IOCTL_HID_GET_STRING:
		ProcessGetString(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)));
		break;
	case IOCTL_HID_READ_REPORT:
#ifndef PASSTHROUGH
		ForwardReadReportRequest(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)));
#else
		ProcessInternalReadReport(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)), OutputBufferLength);
#endif
		break;
//#ifdef PASSTHROUGH
	case IOCTL_HID_SET_OUTPUT_REPORT:
		ProcessSetOutputReport(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)), OutputBufferLength);
		break;
//#endif
	case IOCTL_WIIMOTE_ADDRESSES:
		ProcessAddresses(Request, GetDeviceContext(WdfIoQueueGetDevice(Queue)));
		break;
	default:
		Trace("Unknown IOCTL: %x", IoControlCode);
		WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
	}
}

VOID
ProcessGetDeviceDescriptor(
	_In_ WDFREQUEST Request
	)
{
	NTSTATUS Status;
    WDFMEMORY Memory;
	
	// Get Memory
	Status = WdfRequestRetrieveOutputMemory(Request, &Memory);
	if(!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	// Copy 
	Status = WdfMemoryCopyFromBuffer(Memory, 0, (PVOID)&HIDDescriptor, HIDDescriptor.bLength);
	if(!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	// Complete Request
	WdfRequestCompleteWithInformation(Request, Status, HIDDescriptor.bLength);
}

VOID
ProcessGetDeviceAttributes(
	_In_ WDFREQUEST Request,
	_In_ PHID_DEVICE_CONTEXT HIDContext
	)
{
	NTSTATUS Status;
    PHID_DEVICE_ATTRIBUTES DeviceAttributes = NULL;
	size_t DeviceAttributesSize = sizeof(HID_DEVICE_ATTRIBUTES);

	// Get Buffer
	Status = WdfRequestRetrieveOutputBuffer(Request, DeviceAttributesSize, (PVOID *)&DeviceAttributes, NULL);
	if(!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	// Fill out
	DeviceAttributes->Size = (ULONG)DeviceAttributesSize;
	DeviceAttributes->VendorID = HIDContext->VendorID;
	DeviceAttributes->ProductID = HIDContext->ProductID;
	DeviceAttributes->VersionNumber = 0x0000;
	
	// Complete Request
	WdfRequestCompleteWithInformation(Request, Status, DeviceAttributesSize);
}

VOID
ProcessGetReportDescriptor(
	_In_ WDFREQUEST Request
	)
{
	NTSTATUS Status;
    WDFMEMORY Memory;
	size_t ReportDescriptorSize = HIDReportDescriptorSize;

	// Get Memory
	Status = WdfRequestRetrieveOutputMemory(Request, &Memory);
	if(!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	// Copy 
	Status = WdfMemoryCopyFromBuffer(Memory, 0, (PVOID)&HIDReportDescriptor, ReportDescriptorSize);
	if(!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	// Complete Request
	WdfRequestCompleteWithInformation(Request, Status, ReportDescriptorSize);
}

VOID ProcessGetString(
	_In_ WDFREQUEST Request,
	_In_ PDEVICE_CONTEXT DeviceContext
)
{
	NTSTATUS Status;
	WDF_REQUEST_PARAMETERS Parameters;
	WDFMEMORY Memory;
	PUNICODE_STRING String = NULL;

	WDF_REQUEST_PARAMETERS_INIT(&Parameters);
	WdfRequestGetParameters(Request, &Parameters);

	Status = WdfRequestRetrieveOutputMemory(Request, &Memory);
	if (!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	switch (((UINT_PTR)Parameters.Parameters.DeviceIoControl.Type3InputBuffer) & 0xFF)
	{
	case HID_STRING_ID_IPRODUCT:
		String = &DeviceContext->BluetoothContext.DeviceNameString;
		break;
	case HID_STRING_ID_IMANUFACTURER:
		WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
		return;
	case HID_STRING_ID_ISERIALNUMBER:
		String = &DeviceContext->BluetoothContext.DeviceAddressString;
		break;
	}

	Status = WdfMemoryCopyFromBuffer(Memory, 0, (PVOID)String->Buffer, String->MaximumLength);
	if (!NT_SUCCESS(Status))
	{
		WdfRequestComplete(Request, Status);
		return;
	}

	// Complete Request
	WdfRequestCompleteWithInformation(Request, Status, String->MaximumLength);
}

VOID
ProcessSetOutputReport(
	_In_ WDFREQUEST Request,
	_In_ PDEVICE_CONTEXT DeviceContext,
	_In_ size_t InputBufferLength
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	BYTE * Buffer = NULL;
	size_t Length = 0;

	Status = WdfRequestRetrieveInputBuffer(Request, 1, &Buffer, &Length);
	if (!NT_SUCCESS(Status))
	{
		TraceStatus("ProcessSetOutputReport:WdfRequestRetrieveInputBuffer: ", Status);
		WdfRequestComplete(Request, Status);
		return;
	}
	Trace("ProcessSetOutputReport Data:");
	PrintBytes(Buffer, Length);

	Status = BluetoothSetOutputReport(Request, DeviceContext, InputBufferLength);
	WdfRequestCompleteWithInformation(Request, Status, InputBufferLength);
}

VOID
ProcessInternalReadReport(
	_In_ WDFREQUEST Request,
	_In_ PDEVICE_CONTEXT DeviceContext,
	_In_ size_t OutputBufferLength
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG_PTR BytesWritten = 0;

	Status = BluetoothReadReportUnused(Request, DeviceContext, OutputBufferLength, &BytesWritten);
	WdfRequestCompleteWithInformation(Request, Status, BytesWritten);
}

VOID 
ForwardReadReportRequest(
	_In_ WDFREQUEST Request,
	_In_ PDEVICE_CONTEXT DeviceContext
	)
{
	ReadIoControlBufferForwardRequest(&DeviceContext->HIDContext.ReadBuffer, Request);
}


NTSTATUS 
HIDRelease(
	_In_ PDEVICE_CONTEXT DeviceContext
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PHID_DEVICE_CONTEXT HIDContext = &(DeviceContext->HIDContext);

	ReadIoControlBufferFlush(&HIDContext->ReadBuffer);

	return Status;
}

VOID 
HIDFillReadBufferCallback(
	_In_ PDEVICE_CONTEXT DeviceContext, 
	_Inout_updates_all_(BufferSize) PVOID Buffer,
	_In_ size_t BufferSize, 
	_Out_ PSIZE_T BytesWritten)
{
	PWIIMOTE_DEVICE_CONTEXT WiimoteContext = &(DeviceContext->WiimoteContext);
	(*BytesWritten) = 0;
	Trace("HIDFillReadBufferCallback");

	switch (WiimoteContext->Settings.Mode)
	{
	case Gamepad:
		ParseWiimoteStateAsGamepad(WiimoteContext, Buffer, BufferSize, BytesWritten);
		break;
	case PassThrough:
		BluetoothReadReport(DeviceContext, Buffer, BufferSize, BytesWritten);
		break;
	case IRMouse:
		ParseWiimoteStateAsIRMouse(&(WiimoteContext->State), Buffer, BufferSize, BytesWritten);
		break;
	case DPadMouse:
		ParseWiimoteStateAsDPadMouse(&(WiimoteContext->State), Buffer, BufferSize, BytesWritten);
		break;
	case GamepadAndIRMouse:
		if (!DeviceContext->HIDContext.GamepadAndIRMouseReportToggleFlag)
		{
			ParseWiimoteStateAsIRMouse(&(WiimoteContext->State), Buffer, BufferSize, BytesWritten);
			DeviceContext->HIDContext.GamepadAndIRMouseReportToggleFlag = !DeviceContext->HIDContext.GamepadAndIRMouseReportToggleFlag;

			ReadIoControlBufferDispatchRequest(&(DeviceContext->HIDContext.ReadBuffer));
		}
		else
		{
			ParseWiimoteStateAsGamepad(WiimoteContext, Buffer, BufferSize, BytesWritten);
			DeviceContext->HIDContext.GamepadAndIRMouseReportToggleFlag = !DeviceContext->HIDContext.GamepadAndIRMouseReportToggleFlag;
		}
		break;
	}
}

VOID 
HIDWiimoteStateUpdated(
	_In_ PDEVICE_CONTEXT DeviceContext
	)
{
#ifndef PASSTHROUGH
	ReadIoControlBufferDispatchRequest(&(DeviceContext->HIDContext.ReadBuffer));
#else
	UNREFERENCED_PARAMETER(DeviceContext);
#endif
}

VOID
ProcessAddresses(
	_In_ WDFREQUEST Request,
	_In_ PDEVICE_CONTEXT DeviceContext
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PHID_MINIPORT_ADDRESSES Addresses = NULL;

	Trace("Adresses recieved");

	Status = WdfRequestRetrieveInputBuffer(Request, sizeof(HID_MINIPORT_ADDRESSES), (PVOID *)&Addresses, NULL);
	if(!NT_SUCCESS(Status))
	{
		TraceStatus("Couldn't retrieve Input Buffer", Status);
		WdfRequestComplete(Request, STATUS_SUCCESS);
		return;
	}

	SetHIDMiniportAddresses(DeviceContext, Addresses);

	WdfRequestComplete(Request, STATUS_SUCCESS);
}


#define PrintIrp(MajorFunction) case MajorFunction: \
Trace(#MajorFunction); \
break;

