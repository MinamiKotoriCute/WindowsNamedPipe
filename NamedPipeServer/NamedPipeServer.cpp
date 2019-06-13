#include "NamedPipeServer.h"

#define PIPE_TIMEOUT 5000


VOID WINAPI ReadCallback(DWORD, DWORD, LPOVERLAPPED);

typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipeInst;
	TCHAR chRequest[BUFSIZE];
	DWORD cbRead;
	TCHAR chReply[BUFSIZE];
	DWORD cbToWrite;
} PIPEINST, * LPPIPEINST;

NamedPipeServer::NamedPipeServer()
{
}


NamedPipeServer::~NamedPipeServer()
{
}

bool NamedPipeServer::listen(const std::string& pipeName)
{
	return listen(pipeName, "");
}

bool NamedPipeServer::listen(const std::string& pipeName, const std::string& serverName)
{
	close();

	m_pipe = CreateNamedPipe(TEXT(("\\\\" + serverName + "\\pipe\\" + pipeName).data()),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	if (m_pipe != INVALID_HANDLE_VALUE) {
		if (ConnectNamedPipe(m_pipe, NULL) != FALSE)   // wait for someone to connect to the pipe
		{
			/*while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				// add terminating zero
				buffer[dwRead] = '\0';

				// do something with data in buffer
				printf("%s", buffer);
			}*/
		}
	}
	else
		m_pipe = NULL;



	return isListening();
}

bool NamedPipeServer::listen2(const std::string& pipeName, const std::string& serverName)
{
	pipeNameFull = "\\\\" + serverName + "\\pipe\\" + pipeName;

	m_connectEvent = CreateEvent(
		NULL,    // default security attribute
		TRUE,    // manual reset event 
		TRUE,    // initial state = signaled 
		NULL);   // unnamed event object  

	if (m_connectEvent == NULL)
	{
		printf("CreateEvent failed with %d.\n", GetLastError());
		return false;
	}
	m_oConnect.hEvent = m_connectEvent;

	return connectToNewClient();
}

bool NamedPipeServer::isListening() const
{
	return m_pipe != NULL;
}

void NamedPipeServer::close()
{
	if (m_pipe) {
		DisconnectNamedPipe(m_pipe);
		m_pipe = NULL;
	}
}

void NamedPipeServer::processEvents()
{
	DWORD dwWait = WaitForSingleObjectEx(
		m_connectEvent,  // event object to wait for 
		INFINITE,       // waits indefinitely 
		TRUE);          // alertable wait enabled 

	switch (dwWait)
	{
		// The wait conditions are satisfied by a completed connect 
		// operation. 
	case 0:
		// If an operation is pending, get the result of the 
		// connect operation. 

		if (fPendingIO)
		{
			DWORD cbRet;
			BOOL fSuccess = GetOverlappedResult(
				m_pipe,     // pipe handle 
				&m_oConnect, // OVERLAPPED structure 
				&cbRet,    // bytes transferred 
				FALSE);    // does not wait 
			if (!fSuccess)
			{
				printf("ConnectNamedPipe (%d)\n", GetLastError());
				return 0;
			}
		}

		// Allocate storage for this instance. 

		lpPipeInst = (LPPIPEINST)GlobalAlloc(
			GPTR, sizeof(PIPEINST));
		if (lpPipeInst == NULL)
		{
			printf("GlobalAlloc failed (%d)\n", GetLastError());
			return 0;
		}

		lpPipeInst->hPipeInst = hPipe;

		// Start the read operation for this client. 
		// Note that this same routine is later used as a 
		// completion routine after a write operation. 

		lpPipeInst->cbToWrite = 0;
		CompletedWriteRoutine(0, 0, (LPOVERLAPPED)lpPipeInst);

		// Create new pipe instance for the next client. 

		fPendingIO = CreateAndConnectInstance(
			&oConnect);
		break;

		// The wait is satisfied by a completed read or write 
		// operation. This allows the system to execute the 
		// completion routine. 

	case WAIT_IO_COMPLETION:
		break;

		// An error occurred in the wait function. 

	default:
	{
		printf("WaitForSingleObjectEx (%d)\n", GetLastError());
		return 0;
	}
	}
}

bool NamedPipeServer::connectToNewClient()
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
	{
		printf("CreateNamedPipe failed with %d.\n", GetLastError());
		return false;
	}

	if (ConnectNamedPipe(m_pipe, &m_oConnect)) {
		printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return false;
	}

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
		return true;
		break;

		// Client is already connected, so signal an event. 
	case ERROR_PIPE_CONNECTED:
		createNewClient();
		break;

		// If an error occurs during the connect operation... 
	default:
	{
		printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return false;
	}
	}

	return false;
}

bool NamedPipeServer::createNewClient()
{
	// Allocate storage for this instance. 
	LPPIPEINST lpPipeInst = (LPPIPEINST)GlobalAlloc(
		GPTR, sizeof(PIPEINST));
	if (lpPipeInst == NULL)
	{
		printf("GlobalAlloc failed (%d)\n", GetLastError());
		return 0;
	}

	lpPipeInst->hPipeInst = m_pipe;


	// Start the read operation for this client. 
	// Note that this same routine is later used as a 
	// completion routine after a write operation. 

	lpPipeInst->cbToWrite = 0;
}



VOID WINAPI ReadCallback(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
	if (dwErr != 0) {
		printf("ReadCallback failed (%lld)\n", dwErr);
		return;
	}

	LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;

	BOOL fRead = ReadFileEx(
		lpPipeInst->hPipeInst,
		lpPipeInst->chRequest,
		BUFSIZE * sizeof(TCHAR),
		(LPOVERLAPPED)lpPipeInst,
		(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadRoutine);
}
