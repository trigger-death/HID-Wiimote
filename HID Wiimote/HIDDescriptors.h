/*

Copyright (C) 2017 Julian L�hr
All rights reserved.

Filename:
	HIDDescriptor.h

Abstract:
	Common Header file.
	Contains common includes and declarations.

*/
#pragma once

#include "HID.h"

#define GAMEPAD_REPORT_ID 0x01
#define IRMOUSE_REPORT_ID 0x02
#define DPADMOUSE_REPORT_ID 0x03

extern CONST HID_REPORT_DESCRIPTOR HIDReportDescriptor[];
extern CONST HID_DESCRIPTOR HIDDescriptor;
extern CONST size_t HIDReportDescriptorSize;

#pragma pack(push, 1)
typedef struct _HID_DIRECT_REPORT {
	UCHAR ReportID;
	UCHAR Data[21];
} HID_DIRECT_REPORT, *PHID_DIRECT_REPORT;

VOID
FORCEINLINE
HID_DIRECT_REPORT_INIT(
	_Out_ PHID_DIRECT_REPORT Report)
{
	RtlZeroMemory(Report, sizeof(HID_DIRECT_REPORT));
}

#pragma pack(push, 1)
typedef struct _HID_GAMEPAD_REPORT {
	UCHAR ReportID;
	UCHAR XAxis;
	UCHAR YAxis;
	UCHAR Buttons[2];
	UCHAR ZAxis;
	UCHAR RXAxis;
	UCHAR RYAxis;
	UCHAR RZAxis;
	UCHAR Hatswitch;
} HID_GAMEPAD_REPORT, * PHID_GAMEPAD_REPORT;

VOID
FORCEINLINE
HID_GAMEPAD_REPORT_INIT(
	_Out_ PHID_GAMEPAD_REPORT Report)
{
	RtlZeroMemory(Report, sizeof(HID_GAMEPAD_REPORT));
	Report->ReportID = GAMEPAD_REPORT_ID;
	Report->XAxis = 0x7F;
	Report->YAxis = 0x7F;
	Report->ZAxis = 0x7F;
	Report->RXAxis = 0x7F;
	Report->RYAxis = 0x7F;
	Report->RZAxis = 0x7F;
}

typedef struct _HID_IRMOUSE_REPORT
{
	UCHAR ReportID;
	UCHAR Buttons;
	USHORT X;
	USHORT Y;
} HID_IRMOUSE_REPORT, * PHID_IRMOUSE_REPORT;

VOID
FORCEINLINE
HID_IRMOUSE_REPORT_INIT(
	_Out_ PHID_IRMOUSE_REPORT Report)
{
	RtlZeroMemory(Report, sizeof(HID_IRMOUSE_REPORT));
	Report->ReportID = IRMOUSE_REPORT_ID;
}

typedef struct _HID_DPADMOUSE_REPORT
{
	UCHAR ReportID;
	UCHAR Buttons;
	UCHAR X;
	UCHAR Y;
} HID_DPADMOUSE_REPORT, *PHID_DPADMOUSE_REPORT;

VOID
FORCEINLINE
HID_DPADMOUSE_REPORT_INIT(
	_Out_ PHID_DPADMOUSE_REPORT Report)
{
	RtlZeroMemory(Report, sizeof(HID_DPADMOUSE_REPORT));
	Report->ReportID = DPADMOUSE_REPORT_ID;
}

#pragma pack(pop)