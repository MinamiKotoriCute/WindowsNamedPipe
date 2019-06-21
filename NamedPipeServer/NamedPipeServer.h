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

	void listen(const std::string& pipeName);

	bool isListening() const noexcept;

	void close() noexcept;

	void processEvents();

	std::function<void(NamedPipeSocket*)> onNewConnection;

private:
	void listen();
	void createNewConnectionAndListen(HANDLE pipe) noexcept;

	HANDLE m_pipe;
	HANDLE m_connectEvent;
	OVERLAPPED m_oConnect;
	std::string pipeNameFull;
};

