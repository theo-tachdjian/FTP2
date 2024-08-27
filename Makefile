all: server client

server:
	g++ -o server src/server.cpp src/commands.cpp src/LPTF_Net/* -lpthread -Wall -Wextra -Werror

client:
	g++ -o client src/client.cpp src/commands.cpp src/LPTF_Net/* -Wall -Wextra -Werror

clean:
	rm -f server.exe & rm -f server
	rm -f client.exe & rm -f client

fclean:
	rm -f server.exe & rm -f server
	rm -f client.exe & rm -f client