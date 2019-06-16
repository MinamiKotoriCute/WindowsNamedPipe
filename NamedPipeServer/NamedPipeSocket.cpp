#include "NamedPipeSocket.h"

#include <iostream>

#include "NamedPipeDefinition.h"


NamedPipeSocket::NamedPipeSocket()
{
}


NamedPipeSocket::~NamedPipeSocket()
{
}

bool NamedPipeSocket::connectToServer(const std::string& pipeName, int timeout)
{
	return connectToServer(pipeName, ".", timeout);
}

bool NamedPipeSocket::connectToServer(const std::string& pipeName, const std::string& serverName, int timeout)
{
	close();

	std::string fullPipeName = "\\\\" + serverName + "\\pipe\\" + pipeName;

	HANDLE hPipe;
	while (1)
	{
		hPipe = CreateFile(
			fullPipeName.data(),   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			FILE_FLAG_OVERLAPPED,              // overlapped mode  
			NULL);          // no template file 

		// Break if the pipe handle is valid.
		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			printf("Could not open pipe. GLE=%d\n", GetLastError());
			return false;
		}

		// All pipe instances are busy, so wait for [timeout] seconds. 
		if (!WaitNamedPipe(fullPipeName.data(), timeout))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return false;
		}
	}

	m_pipeInstance = (LPPIPEINST)GlobalAlloc(
		GPTR, sizeof(PIPEINST));
	if (m_pipeInstance == NULL)
	{
		printf("GlobalAlloc failed (%d)\n", GetLastError());
		return 0;
	}

	m_pipeInstance->hPipeInst = hPipe;
	m_pipeInstance->instance = this;

	// set ready read callback
	m_readBuffer.resize(BUFSIZE);
	setReadyReadCallback();

	return isOpen();
}

void NamedPipeSocket::write(const char* data)
{
	// include terminating null character
	write(data, std::strlen(data) + 1);
}

void NamedPipeSocket::write(const char* data, std::size_t size)
{
	if (!isOpen()) {
		std::clog << "use " __FUNCTION__ " fail. Because named pipe not open." << std::endl;
		return;
	}

	DWORD dwWritten;
	WriteFile(m_pipeInstance->hPipeInst,
		data,
		size,   // = length of string + terminating '\0' !!!
		&dwWritten,
		NULL);
}

void NamedPipeSocket::close()
{
	if (isOpen()) {
		CloseHandle(m_pipeInstance->hPipeInst);
		m_pipeInstance->hPipeInst = NULL;

		GlobalFree(m_pipeInstance);
		m_pipeInstance = NULL;
	}
}

bool NamedPipeSocket::isOpen() const
{
	return m_pipeInstance != NULL;
}

VOID __stdcall NamedPipeSocket::readyRead(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
	LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;
	NamedPipeSocket* instance = (NamedPipeSocket*)lpPipeInst->instance;

	if (dwErr == 0 && instance->onReadyRead) {
		instance->onReadyRead(&instance->m_readBuffer[0], cbBytesRead);
	}
}

void NamedPipeSocket::setReadyReadCallback()
{
	BOOL fRead = ReadFileEx(
		m_pipeInstance->hPipeInst,
		&m_readBuffer[0],
		m_readBuffer.size(),
		(LPOVERLAPPED)m_pipeInstance,
		(LPOVERLAPPED_COMPLETION_ROUTINE)&NamedPipeSocket::readyRead);

	if (!fRead) {
		printf("GG\n");
		return;
	}

	switch (GetLastError())
	{
	case ERROR_SUCCESS:
		break;
	default:
		printf("GGGG %d", GetLastError());
		break;
	}
}
