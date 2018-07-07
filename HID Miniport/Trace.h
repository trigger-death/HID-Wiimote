/*

Copyright (C) 2013 Julian L�hr
All rights reserved.

Filename:
	Trace.h

Abstract:
	Header file for Trace.c
	Contains declarations and configuration for Tracing.

*/
#ifndef _TRACE_H_
#define _TRACE_H_

#include <ntddk.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

VOID
Trace(
    _In_ PCCHAR  DebugMessage,
    ...
    );

VOID
TraceStatus(
	_In_ PCCHAR  DebugMessage,
	_In_ NTSTATUS Status
);

VOID
PrintBytes(
	_In_reads_(Size) PCUCHAR Data,
	_In_ SIZE_T Size
	);

#endif
