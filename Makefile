all: server client

server:
	g++ -o lpf_server src/server.cpp src/server_actions.cpp src/file_utils.cpp src/LPTF_Net/* -lpthread -lstdc++fs -std=c++17 -Wall -Wextra -Werror

client:
	g++ -o lpf src/client.cpp src/client_actions.cpp src/file_utils.cpp src/LPTF_Net/* -lstdc++fs -std=c++17 -Wall -Wextra -Werror

clean:
	rm -f lpf_server.exe & rm -f lpf_server
	rm -f lpf.exe & rm -f lpf

fclean:
	rm -f lpf_server.exe & rm -f lpf_server
	rm -f lpf.exe & rm -f lpf

test:
	g++ -o test src/test.cpp src/file_utils.cpp src/LPTF_Net/* -lstdc++fs -std=c++17 -Wall -Wextra -Werror
ctest:
	rm -f test.exe & rm -f test