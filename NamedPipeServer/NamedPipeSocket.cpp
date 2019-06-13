#include "NamedPipeSocket.h"

#include <iostream>


NamedPipeSocket::NamedPipeSocket()
{
}


NamedPipeSocket::~NamedPipeSocket()
{
}

bool NamedPipeSocket::connectToServer(const std::string& pipeName)
{
}

bool NamedPipeSocket::connectToServer(const std::string& pipeName, const std::string& serverName)
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
			0,              // default attributes 
			NULL);          // no template file 

	  // Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			printf("Could not open pipe. GLE=%d\n", GetLastError());
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipe(fullPipeName.data(), 20000))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return -1;
		}
	}

	return m_pipe != NULL;
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
	WriteFile(m_pipe,
		data,
		size,   // = length of string + terminating '\0' !!!
		&dwWritten,
		NULL);
}

void NamedPipeSocket::close()
{
	if (isOpen()) {
		CloseHandle(m_pipe);
		m_pipe = 0;
	}
}

bool NamedPipeSocket::isOpen() const
{
	return m_pipe != NULL;
}
