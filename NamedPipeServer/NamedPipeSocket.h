#pragma once

#include <Windows.h>

#include <string>
#include <vector>
#include <functional>

#include "NamedPipeDefinition.h"

class NamedPipeSocket
{
public:
	NamedPipeSocket();
	NamedPipeSocket(HANDLE pipe);
	~NamedPipeSocket();

	bool connectToServer(const std::string& pipeName, int timeout = 5000);
	bool connectToServer(const std::string& pipeName, const std::string& serverName, int timeout = 5000);

	void write(const char* data);
	void write(const char* data, std::size_t size);

	// close the named pipe
	void close();

	// check the named pipe is connected
	bool isOpen() const;

	std::function<void(const char* data, std::size_t size)> onReadyRead;
	std::function<void()> onWriteComplete;
	std::function<void()> onDisconnected;

private:
	static VOID WINAPI readyRead(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
	static VOID WINAPI writeComplete(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap);
	void _readyRead(std::size_t readSize);
	void _writeComplete();
	void _disconnected();
	void setReadyReadCallback();
	void createPipeInstance(HANDLE pipe);

	LPPIPEINST m_pipeInstance;
	bool isServerEnd;
	std::vector<char> m_readBuffer;
};

