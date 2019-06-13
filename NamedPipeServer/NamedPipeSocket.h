#pragma once

#include <Windows.h>

#include <string>

class NamedPipeSocket
{
public:
	NamedPipeSocket();
	~NamedPipeSocket();

	bool connectToServer(const std::string& pipeName, int timeout = 5000);
	bool connectToServer(const std::string& pipeName, const std::string& serverName, int timeout = 5000);

	void write(const char* data);
	void write(const char* data, std::size_t size);

	// close the named pipe
	void close();

	// check the named pipe is connected
	bool isOpen() const;

private:
	static VOID WINAPI readyRead(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);

	HANDLE m_pipe;
};

