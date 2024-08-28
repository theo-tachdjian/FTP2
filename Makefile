all: server client

server:
	g++ -o lpf_server src/server.cpp src/commands.cpp src/LPTF_Net/* -lpthread -lstdc++fs -Wall -Wextra -Werror

client:
	g++ -o lpf src/client.cpp src/commands.cpp src/LPTF_Net/* -lstdc++fs -Wall -Wextra -Werror

clean:
	rm -f lpf_server.exe & rm -f lpf_server
	rm -f lpf.exe & rm -f lpf

fclean:
	rm -f lpf_server.exe & rm -f lpf_server
	rm -f lpf.exe & rm -f lpf