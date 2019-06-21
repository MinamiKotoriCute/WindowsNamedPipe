#include "NamedPipeSocket.h"

#include <iostream>

#include "NamedPipeDefinition.h"


NamedPipeSocket::NamedPipeSocket()
{
	isServerEnd = false;
}

NamedPipeSocket::NamedPipeSocket(HANDLE pipe)
{
	isServerEnd = true;
	createPipeInstance(pipe);
}


NamedPipeSocket::~NamedPipeSocket()
{
	close();
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
			printf("Could not open pipe. GetLastError=%d\n", GetLastError());
			return false;
		}

		// All pipe instances are busy, so wait for [timeout] seconds. 
		if (!WaitNamedPipe(fullPipeName.data(), timeout))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return false;
		}
	}

	createPipeInstance(hPipe);

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
	WriteFileEx(
		m_pipeInstance->hPipeInst,
		data,
		size,
		(LPOVERLAPPED)m_pipeInstance,
		(LPOVERLAPPED_COMPLETION_ROUTINE)& NamedPipeSocket::writeComplete);
}

void NamedPipeSocket::close()
{
	if (isOpen()) {
		if(isServerEnd && !DisconnectNamedPipe(m_pipeInstance->hPipeInst))
			printf("DisconnectNamedPipe failed with %d.\n", GetLastError());

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

	switch (dwErr)
	{
	case ERROR_SUCCESS:
		instance->_readyRead(cbBytesRead);
		break;

	case ERROR_BROKEN_PIPE:
		instance->_disconnected();
		break;

	default:
		printf("writeComplete GetLastError(): %d", GetLastError());
		break;
	}
}

VOID __stdcall NamedPipeSocket::writeComplete(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap)
{
	LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;
	NamedPipeSocket* instance = (NamedPipeSocket*)lpPipeInst->instance;

	switch (dwErr)
	{
	case ERROR_SUCCESS:
		instance->_writeComplete();
		break;

	case ERROR_BROKEN_PIPE:
		instance->_disconnected();
		break;

	default:
		printf("writeComplete GetLastError(): %d", GetLastError());
		break;
	}
}

void NamedPipeSocket::_readyRead(std::size_t readSize)
{
	onReadyRead(&m_readBuffer[0], readSize);
	setReadyReadCallback();
}

void NamedPipeSocket::_writeComplete()
{
	if (onWriteComplete)
		onWriteComplete();
}

void NamedPipeSocket::_disconnected()
{
	if (onDisconnected)
		onDisconnected();
}

void NamedPipeSocket::setReadyReadCallback()
{
	if (!isOpen())
		return;

	BOOL fRead = ReadFileEx(
		m_pipeInstance->hPipeInst,
		&m_readBuffer[0],
		m_readBuffer.size(),
		(LPOVERLAPPED)m_pipeInstance,
		(LPOVERLAPPED_COMPLETION_ROUTINE)&NamedPipeSocket::readyRead);

	if (!fRead) {
		close();
		return;
	}

	switch (GetLastError())
	{
	case ERROR_SUCCESS:
		break;
	case ERROR_IO_PENDING:
		break;
	default:
		printf("ReadFileEx %d", GetLastError());
		break;
	}
}

void NamedPipeSocket::createPipeInstance(HANDLE pipe)
{
	m_pipeInstance = (LPPIPEINST)GlobalAlloc(
		GPTR, sizeof(PIPEINST));
	if (m_pipeInstance == NULL)
	{
		printf("GlobalAlloc failed (%d)\n", GetLastError());
		return;
	}

	m_pipeInstance->hPipeInst = pipe;
	m_pipeInstance->instance = this;

	// set ready read callback
	m_readBuffer.resize(BUFSIZE);
	setReadyReadCallback();
}
