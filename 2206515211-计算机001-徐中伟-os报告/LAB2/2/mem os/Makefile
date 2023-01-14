main : main.o memory_manager.o
	g++ main.o memory_manager.o -o main

main.o : main.cpp
	g++ -c main.cpp

memory_manager.o : memory_manager.cpp
	g++ -c memory_manager.cpp

clean:
	rm *.o run
