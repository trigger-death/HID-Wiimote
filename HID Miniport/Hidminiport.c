/*

Copyright (C) 2014 Julian Löhr
All rights reserved.

Filename:
	hidminiport.c

Abstract:
	Actual miniport driver for HIDClass. Passes through all IRPs. Additonally sends down a custiom IOCTL, with the actual FDO and HidNotifyPrcense address.

*/

#include "hidminiport.h"

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS Status;
	HID_MINIDRIVER_REGISTRATION HIDMinidriverRegistration;
    ULONG i;

	//Trace("Driver Entry");

	//Set Function Pointers for IRPs
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = PassThrough;
    }

	//Special for PnP
	DriverObject->MajorFunction[IRP_MJ_PNP] = PnPPassThrough;

    DriverObject->DriverExtension->AddDevice = AddDevice;
    DriverObject->DriverUnload = Unload;

	//Register as HID Minidriver
    RtlZeroMemory(&HIDMinidriverRegistration, sizeof(HIDMinidriverRegistration));

    HIDMinidriverRegistration.Revision            = HID_REVISION;
    HIDMinidriverRegistration.DriverObject        = DriverObject;
    HIDMinidriverRegistration.RegistryPath        = RegistryPath;
    HIDMinidriverRegistration.DeviceExtensionSize = 0;
    HIDMinidriverRegistration.DevicesArePolled = FALSE;

    Status = HidRegisterMinidriver(&HIDMinidriverRegistration);
    if (!NT_SUCCESS(Status) ){
        return Status;
    }

    return Status;
}


NTSTATUS
AddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT FunctionalDeviceObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
	
	//Trace("Add Device");

    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

#define PrintIrp(MajorFunction) case MajorFunction: \
Trace(#MajorFunction); \
break;

NTSTATUS
PassThrough(
    _In_    PDEVICE_OBJECT  DeviceObject,
    _Inout_ PIRP            Irp
    )
{
	//Trace("Passthrough");
	PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(Irp);
	//Trace("PassThrough: %x %x %i", StackLocation->MajorFunction, StackLocation->MinorFunction, StackLocation->Parameters.DeviceIoControl.IoControlCode);

	switch (StackLocation->MajorFunction) {
	case IRP_MJ_WRITE:
		Trace("IRP_MJ_WRITE");
		if (StackLocation->DeviceObject->Flags & DO_BUFFERED_IO) {
			Trace("IRP_MJ_WRITE bDO_BUFFERED_IO %p", Irp->AssociatedIrp.SystemBuffer);
			//PrintBytes(Irp->AssociatedIrp.SystemBuffer, StackLocation->Parameters.Write.Length);
		}
		else {
			Trace("IRP_MJ_WRITE DO_DIRECT_IO %p", Irp->UserBuffer);
			//PrintBytes(Irp->AssociatedIrp.SystemBuffer, StackLocation->Parameters.Write.Length);
		}
		break;
	case IRP_MJ_READ:
		Trace("IRP_MJ_READ");
		break;
	case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		//Trace("IRP_MJ_DEVICE_CONTROL: %i", StackLocation->Parameters.DeviceIoControl.IoControlCode);
		switch (StackLocation->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_HID_READ_REPORT:
			Trace("IRP_MJ_INTERNAL_DEVICE_CONTROL: IOCTL_HID_READ_REPORT");
			break;
		case IOCTL_HID_WRITE_REPORT:
			Trace("IRP_MJ_INTERNAL_DEVICE_CONTROL: IOCTL_HID_WRITE_REPORT");
			break;
		case IOCTL_HID_SET_OUTPUT_REPORT:
			Trace("IRP_MJ_INTERNAL_DEVICE_CONTROL: IOCTL_HID_SET_OUTPUT_REPORT");
			break;
		case IOCTL_HID_GET_INPUT_REPORT:
			Trace("IRP_MJ_INTERNAL_DEVICE_CONTROL: IOCTL_HID_GET_INPUT_REPORT");
			break;
		}
	case IRP_MJ_DEVICE_CONTROL:
		//Trace("IRP_MJ_DEVICE_CONTROL: %i", StackLocation->Parameters.DeviceIoControl.IoControlCode);
		switch (StackLocation->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_HID_READ_REPORT:
			Trace("IRP_MJ_DEVICE_CONTROL: IOCTL_HID_READ_REPORT");
			break;
		case IOCTL_HID_WRITE_REPORT:
			Trace("IRP_MJ_DEVICE_CONTROL: IOCTL_HID_WRITE_REPORT");
			break;
		case IOCTL_HID_SET_OUTPUT_REPORT:
			Trace("IRP_MJ_DEVICE_CONTROL: IOCTL_HID_SET_OUTPUT_REPORT");
			break;
		case IOCTL_HID_GET_INPUT_REPORT:
			Trace("IRP_MJ_DEVICE_CONTROL: IOCTL_HID_GET_INPUT_REPORT");
			break;
		}
		break;
		PrintIrp(IRP_MJ_CLEANUP);
		PrintIrp(IRP_MJ_CLOSE);
		PrintIrp(IRP_MJ_CREATE);
		PrintIrp(IRP_MJ_FILE_SYSTEM_CONTROL);
		PrintIrp(IRP_MJ_FLUSH_BUFFERS);
		PrintIrp(IRP_MJ_POWER);
		PrintIrp(IRP_MJ_QUERY_INFORMATION);
		PrintIrp(IRP_MJ_SET_INFORMATION);
		PrintIrp(IRP_MJ_SHUTDOWN);
		PrintIrp(IRP_MJ_SYSTEM_CONTROL);
	}
	
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
}

/*
NTSTATUS SendAddressesCompeted(
  _In_      PDEVICE_OBJECT DeviceObject,
  _In_      PIRP Irp,
  _In_opt_  PVOID Context
)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Trace("Send Addresses Completed");

	if(Context != NULL)
	{
		ExFreePoolWithTag(Context, IOCTL_POOL_TAG);
	}

	IoFreeIrp(Irp);

	return STATUS_SUCCESS;
}
*/

NTSTATUS
SendAddresses(
    _In_    PDEVICE_OBJECT  DeviceObject
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PIRP NewIrp;
	IO_STATUS_BLOCK StatusBlock;
	HID_MINIPORT_ADDRESSES AddressesBuffer;

	UNREFERENCED_PARAMETER(DeviceObject);

	Trace("Sending Addresses!");

	/*AddressesBuffer = (PHID_MINIPORT_ADDRESSES)ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_MINIPORT_ADDRESSES), IOCTL_POOL_TAG);
	if(AddressesBuffer == NULL)
	{
		Trace("Couldn't allocate Addresses Buffer");
		return STATUS_SUCCESS;
	}
	
	RtlSecureZeroMemory(AddressesBuffer, sizeof(HID_MINIPORT_ADDRESSES));
	*/
	AddressesBuffer.FDO = DeviceObject;
	AddressesBuffer.HidNotifyPresence = HidNotifyPresence;

	NewIrp = IoBuildDeviceIoControlRequest(IOCTL_WIIMOTE_ADDRESSES, GET_NEXT_DEVICE_OBJECT(DeviceObject), &AddressesBuffer, sizeof(HID_MINIPORT_ADDRESSES), NULL, 0, TRUE, NULL, &StatusBlock);
	if(NewIrp == NULL)
	{
		Trace("Couldn't Build IRP");
		return Status;
	}

	//IoSetCompletionRoutine(NewIrp, SendAddressesCompeted, AddressesBuffer, TRUE, TRUE, TRUE);

	Status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), NewIrp);
	if(!NT_SUCCESS(Status))
	{
		if(Status == STATUS_PENDING)
		{
			Trace("SendAddress IoCallDriver is Pending!");
		}
		else
		{
			TraceStatus("SendAddress IoCallDriver", Status);
		}

		return Status;
	}
	
	
	Trace("Sending Addresses returned!");

	return Status;
}

NTSTATUS
PnPPassThrough(
    _In_    PDEVICE_OBJECT  DeviceObject,
    _Inout_ PIRP            Irp
    )
{
	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(Irp);
	UCHAR MinorFunction = StackLocation->MinorFunction;
	Trace("PnPPassThrough MinorFunction: %x", MinorFunction);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    Status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
	if(!NT_SUCCESS(Status))
	{
		return Status;
	}
	
	if(MinorFunction == IRP_MN_START_DEVICE)
	{
		Trace("Device Start");

		Status = SendAddresses(DeviceObject);
		if(!NT_SUCCESS(Status))
		{
			return Status;
		}
	}

	return Status;
}



VOID
Unload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    return;
}

