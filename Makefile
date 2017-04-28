#TODO spellcheck
#TODO boilerplate

#The compiler
CXX = g++ -g -pthread -std=c++0x -Wall

#Objects needed to build the sender and receiver
server_objects = ./build/server.o ./build/file_layer.o ./build/udp_socket.o \
				 ./build/jstp_segment.o ./build/jstp_streams.o
client_objects = ./build/client.o ./build/file_layer.o ./build/udp_socket.o \
				 ./build/jstp_segment.o ./build/jstp_streams.o

#Make all, the default
all : ./bin/server ./bin/client
.PHONY: all

#Make the sender
./bin/server: $(server_objects)
	$(CXX) $(server_objects) -o $@

#Make the receiver
./bin/client: $(client_objects)
	$(CXX) $(client_objects) -o $@

#Make the objects
./build/server.o : ./src/server.main.cpp ./src/file_layer.hpp
	$(CXX) -c ./src/server.main.cpp -o $@

./build/client.o : ./src/client.main.cpp ./src/file_layer.hpp
	$(CXX) -c ./src/client.main.cpp -o $@

./build/file_layer.o : ./src/file_layer.hpp ./src/file_layer.cpp
	$(CXX) -c ./src/file_layer.cpp -o $@

./build/udp_socket.o : ./src/udp_socket.cpp ./src/udp_socket.hpp
	$(CXX) -c ./src/udp_socket.cpp -o $@

./build/jstp_segment.o : ./src/jstp_segment.hpp ./src/jstp_segment.cpp
	$(CXX) -c ./src/jstp_segment.cpp -o $@

./build/jstp_streams.o : ./src/jstp_streams.cpp ./src/jstp_streams.hpp
	$(CXX) -c ./src/jstp_streams.cpp -o $@

.PHONY: clean
clean :
	rm ./bin/* ./build/*
