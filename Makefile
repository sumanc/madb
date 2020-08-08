CC = g++
CFLAGS = -I . 
LIBS = 
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
   LIBS = -lrt -lpthread
endif

MADB_OBJECTS = madb/madb.cpp portfinder.cpp
DEVICEPORT_OBJECTS = deviceport/deviceport.cpp portfinder.cpp

all: madb deviceport


madb: $(MADB_OBJECTS)
	@echo $(LIBS)
	$(CC) $(MADB_OBJECTS) $(CFLAGS) $(LIBS) -o out/madb

deviceport: $(DEVICEPORT_OBJECTS)
	$(CC) $(DEVICEPORT_OBJECTS) $(CFLAGS) $(LIBS) -o out/deviceport

clean:
	rm -f *.o out/madb out/deviceport

