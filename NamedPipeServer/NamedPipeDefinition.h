#pragma once

#include <Windows.h>

#define BUFSIZE 4096

typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipeInst;
	void* instance;
} PIPEINST, * LPPIPEINST;

