/*

Copyright (C) 2017 Julian L�hr
All rights reserved.

Filename:
	HIDWiimote.h

Abstract:
	Common Header file.
	Contains common includes and declarations.

*/
#pragma once

#include <initguid.h>
#include <ntddk.h>
#include <wdf.h>

#include "Trace.h"

//#define GAMEPAD
//#define MOUSE_DPAD
//#define MOUSE_IR
#define PASSTHROUGH

// No HID Report Descriptors
//#define GAMEPAD_MOUSE_IR


//Forward Declarations
typedef struct _DEVICE_CONTEXT DEVICE_CONTEXT, * PDEVICE_CONTEXT;
typedef struct _DEVICE_INTERFACE_CONTEXT DEVICE_INTERFACE_CONTEXT, *PDEVICE_INTERFACE_CONTEXT;
typedef struct _WIIMOTE_DEVICE_CONTEXT WIIMOTE_DEVICE_CONTEXT, * PWIIMOTE_DEVICE_CONTEXT;
typedef struct _BLUETOOTH_DEVICE_CONTEXT BLUETOOTH_DEVICE_CONTEXT, * PBLUETOOTH_DEVICE_CONTEXT;
typedef struct _HID_DEVICE_CONTEXT HID_DEVICE_CONTEXT, * PHID_DEVICE_CONTEXT;

// Used by HID Layer and Device Part
typedef NTSTATUS (*PNOTIFY_PRESENCE)(PDEVICE_OBJECT, BOOLEAN);

typedef struct _HID_MINIPORT_ADDRESSES 
{
	PDEVICE_OBJECT FDO;
	PNOTIFY_PRESENCE HidNotifyPresence;

} HID_MINIPORT_ADDRESSES, * PHID_MINIPORT_ADDRESSES;
