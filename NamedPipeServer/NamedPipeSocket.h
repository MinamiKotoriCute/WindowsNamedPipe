#pragma once

#include <Windows.h>

#include <string>

class NamedPipeSocket
{
public:
	NamedPipeSocket();
	~NamedPipeSocket();

	bool connectToServer(const std::string& pipeName);
	bool connectToServer(const std::string& pipeName, const std::string& serverName);

	void write(const char* data);
	void write(const char* data, std::size_t size);

	// close the named pipe
	void close();

	// check the named pipe is connected
	bool isOpen() const;

private:
	HANDLE m_pipe;
};

