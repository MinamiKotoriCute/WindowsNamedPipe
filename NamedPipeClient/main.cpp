#include <iostream>
#include <Windows.h>
#include "../NamedPipeServer/NamedPipeSocket.h"


int main(int argc, char* argv[])
{
	NamedPipeSocket socket;
	socket.onReadyRead = [&socket](const char *data, std::size_t size) {
		std::cout << "receive (" << size << "):" << data;
		socket.close();
	};

	if (socket.connectToServer("mynamedpipe") == false) {
		std::cout << "connect to server false" << std::endl;
		return -1;
	}

	const char msg[] = "hello";
	std::cout << msg << std::endl;
	socket.write(msg);

	while (socket.isOpen()) {
		SleepEx(100, true);
	}

	return 0;
}