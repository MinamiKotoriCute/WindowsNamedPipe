#include <iostream>
#include <thread>

#include "../NamedPipeServer/NamedPipeSocket.h"


int main(int argc, char* argv[])
{
	NamedPipeSocket socket;
	const char msg[] = "hello";
	int askCount = 100;
	auto send = [&socket, msg]() {
		std::cout << "Send:" << msg << std::endl;
		socket.write(msg);
	};

	socket.onReadyRead = [&socket, &askCount, &send](const char *data, std::size_t size) {
		std::cout << "Receive (" << size << "):" << data << std::endl;

		if (--askCount < 0)
			socket.close();
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			send();
		}
	};

	if (socket.connectToServer("mynamedpipe") == false) {
		std::cout << "connect to server false" << std::endl;
		return -1;
	}

	send();

	while (socket.isOpen()) {
		SleepEx(100, true);
	}

	return 0;
}