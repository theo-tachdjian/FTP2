COMMON_FILES = src/file_utils.cpp src/LPTF_Net/*
COMPILER_FLAGS = -Wall -Wextra -Werror

all: server client

server:
	g++ -o lpf_server src/server.cpp src/server_actions.cpp $(COMMON_FILES) src/logger.cpp -lpthread -lstdc++fs -std=c++17 $(COMPILER_FLAGS)

client:
	g++ -o lpf src/client.cpp src/client_actions.cpp $(COMMON_FILES) -lstdc++fs -std=c++17 $(COMPILER_FLAGS)

clean:
	rm -f lpf_server.exe & rm -f lpf_server
	rm -f lpf.exe & rm -f lpf

fclean:
	rm -f lpf_server.exe & rm -f lpf_server
	rm -f lpf.exe & rm -f lpf

test:
	g++ -o test src/test.cpp $(COMMON_FILES) src/logger.cpp -lpthread -lstdc++fs -std=c++17 $(COMPILER_FLAGS)
ctest:
	rm -f test.exe & rm -f test