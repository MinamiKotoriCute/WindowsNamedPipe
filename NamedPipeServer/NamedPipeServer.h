#pragma once

#include <string>
#include <functional>
#include <Windows.h>

#include "NamedPipeSocket.h"

class NamedPipeServer
{
public:
	NamedPipeServer();
	~NamedPipeServer();

	bool listen(const std::string& pipeName);
	bool listen(const std::string& pipeName, const std::string &serverName);
	bool listen2(const std::string& pipeName, const std::string& serverName);

	// check currently listening for incoming connections
	bool isListening() const;

	void close();

	void processEvents();

private:
	// return is pending IO
	bool connectToNewClient();
	bool createNewClient();



	std::function<void(NamedPipeSocket*)> newConnectionHandle;
	HANDLE m_pipe;
	HANDLE m_connectEvent;
	OVERLAPPED m_oConnect;
	std::string pipeNameFull;
};

