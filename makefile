EXECUTE = mtServer mtClient
OBJECTS = mtServer.o mtClient.o
all: mtServer mtClient
CXX = g++
CXXFLAGS = -ggdb
mtServer: mtServer.o
	$(CXX) mtServer.o -lpthread -o mtServer
mtServer.o: mtServer.c mtServer.h
	$(CXX) $(CXXFLAGS) -c mtServer.c
mtClient: mtClient.o
	$(CXX) mtClient.o -lpthread -o mtClient
mtClient.o: mtClient.c mtClient.h
	$(CXX) $(CXXFLAGS) -c mtClient.c
	
clean:
	rm -rf $(OBJECTS)
	rm -rf $(EXECUTE)
	
strip:
	strip $(EXECUTE) 
