#include "NamedPipeServer.h"

#include <system_error>
#include "NamedPipeDefinition.h"

#define PIPE_TIMEOUT 5000


VOID WINAPI ReadCallback(DWORD, DWORD, LPOVERLAPPED);

NamedPipeServer::NamedPipeServer()
{
}


NamedPipeServer::~NamedPipeServer()
{
	close();
}

void NamedPipeServer::listen(const std::string& pipeName)
{
	close();

	pipeNameFull = "\\\\.\\pipe\\" + pipeName;

	m_connectEvent = CreateEvent(
		NULL,    // default security attribute
		TRUE,    // manual reset event 
		TRUE,    // initial state = signaled 
		NULL);   // unnamed event object  

	if (m_connectEvent == NULL)
		throw std::runtime_error(std::string("CreateEvent failed with " + std::to_string(GetLastError())) + ".\n");

	m_oConnect.hEvent = m_connectEvent;

	listen();
}

bool NamedPipeServer::isListening() const noexcept
{
	return m_pipe != NULL;
}

void NamedPipeServer::close() noexcept
{
	if (m_pipe) {
		DisconnectNamedPipe(m_pipe);
		CloseHandle(m_pipe);
		m_pipe = NULL;
	}

	if (m_connectEvent) {
		CloseHandle(m_connectEvent);
		m_connectEvent = NULL;
	}
}

void NamedPipeServer::processEvents()
{
	while (true) {
		DWORD dwWait = WaitForSingleObjectEx(
			m_connectEvent,  // event object to wait for 
			INFINITE,       // waits indefinitely 
			TRUE);          // alertable wait enabled 

		switch (dwWait)
		{
			// The wait conditions are satisfied by a completed connect 
			// operation. 
		case WAIT_OBJECT_0: {
			// If an operation is pending, get the result of the 
			// connect operation. 

			DWORD cbRet;
			BOOL fSuccess = GetOverlappedResult(
				m_pipe,     // pipe handle 
				&m_oConnect, // OVERLAPPED structure 
				&cbRet,    // bytes transferred 
				FALSE);    // does not wait 
			if (!fSuccess)
			{
				printf("ConnectNamedPipe (%d)\n", GetLastError());
				return;
			}

			createNewConnectionAndListen(m_pipe);
			break;
		}
		case WAIT_IO_COMPLETION:
			break;

			// An error occurred in the wait function. 
		default:
			throw std::runtime_error(std::string("WaitForSingleObjectEx (" + std::to_string(GetLastError()) + ")"));
		}
	}
}

void NamedPipeServer::listen()
{
	m_pipe = CreateNamedPipe(
		pipeNameFull.data(),             // pipe name 
		PIPE_ACCESS_DUPLEX |      // read/write access 
		FILE_FLAG_OVERLAPPED,     // overlapped mode 
		PIPE_TYPE_MESSAGE |       // message-type pipe 
		PIPE_READMODE_MESSAGE |   // message read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // unlimited instances 
		BUFSIZE * sizeof(TCHAR),    // output buffer size 
		BUFSIZE * sizeof(TCHAR),    // input buffer size 
		PIPE_TIMEOUT,             // client time-out 
		NULL);                    // default security attributes

	if (m_pipe == INVALID_HANDLE_VALUE)
		throw std::runtime_error(std::string("CreateNamedPipe failed with " + std::to_string(GetLastError())) + ".\n");

	if (ConnectNamedPipe(m_pipe, &m_oConnect))
		throw std::runtime_error(std::string("ConnectNamedPipe failed with " + std::to_string(GetLastError())) + ".\n");

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
		break;

		// Client is already connected, so signal an event. 
	case ERROR_PIPE_CONNECTED:
		createNewConnectionAndListen(m_pipe);
		break;

		// If an error occurs during the connect operation... 
	default:
		throw std::runtime_error(std::string("ConnectNamedPipe failed with " + std::to_string(GetLastError())) + ".\n");
	}
}

void NamedPipeServer::createNewConnectionAndListen(HANDLE pipe) noexcept
{
	onNewConnection(new NamedPipeSocket(m_pipe));
	m_pipe = NULL;
	listen();
}
