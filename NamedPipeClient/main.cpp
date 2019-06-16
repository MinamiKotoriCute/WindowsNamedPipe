#include <iostream>
#include <Windows.h>
#include "../NamedPipeServer/NamedPipeSocket.h"


void listenWinEvent()
{
	while(true)
		SleepEx(100, true);
}

int main(int argc, char* argv[])
{
	NamedPipeSocket s;
	s.onReadyRead = [](const char *data, std::size_t size) {
		std::cout << "receive (" << size << "):" << data;
	};


	if (s.connectToServer("mynamedpipe") == false) {
		std::cout << "connect to server false" << std::endl;
		return -1;
	}

	s.write("asd");

	listenWinEvent();

	return 0;
}