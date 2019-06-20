#include <iostream>

#include "NamedPipeServer.h"

int main()
{
	NamedPipeServer server;
	int currentConnectionNumber = 0;

	server.onNewConnection = ([&currentConnectionNumber](NamedPipeSocket * socket) {
		std::cout << "New Connection. Current connection number:" << ++currentConnectionNumber << std::endl;

		socket->onReadyRead = [socket, &currentConnectionNumber](const char *data, std::size_t size) {
			if(std::string(data) == "hello")
				socket->write(("Current connection number:" + std::to_string(currentConnectionNumber)).data());
			else
				socket->write("I need you send hello to me.");
		};
		socket->onDisconnected = [socket, &currentConnectionNumber]() {
			std::cout << "Disconnected. Current connection number:" << --currentConnectionNumber << std::endl;
			delete socket;
		};
	});

	server.listen("mynamedpipe");
	server.processEvents();
}