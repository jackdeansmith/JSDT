#TODO spellcheck
#TODO boilerplate

#The compiler
CXX = g++ -std=c++0x -Wall

#Objects needed to build the sender and receiver
server_objects = ./build/server.o ./build/jstp_message.o
client_objects = ./build/client.o ./build/jstp_message.o

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
./build/server.o : ./src/server.main.cpp ./src/jstp_message.hpp
	$(CXX) -c ./src/server.main.cpp -o $@

./build/client.o : ./src/client.main.cpp ./src/jstp_message.hpp
	$(CXX) -c ./src/client.main.cpp -o $@

./build/jstp_message.o : ./src/jstp_message.hpp ./src/jstp_message.cpp
	$(CXX) -c ./src/jstp_message.cpp -o $@

.PHONY: clean
clean :
	rm ./bin/* ./build/*
