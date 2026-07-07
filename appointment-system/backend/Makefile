CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I. -Iinclude -Isrc
LDFLAGS = -lpthread -lws2_32

TARGET = appointment_server.exe

SRCS = src/main.cpp \
       src/auth.cpp \
       src/config/config.cpp \
       src/middleware/AuthMiddleware.cpp \
       src/services/DatabaseService.cpp \
       src/controllers/AuthController.cpp \
       src/controllers/ProviderController.cpp \
       src/controllers/ServiceController.cpp \
       src/controllers/AppointmentController.cpp \
       src/controllers/ReviewController.cpp \
       src/controllers/NotificationController.cpp \
       src/controllers/StatsController.cpp \
       src/sqlite3.c

OBJS = src/main.o \
       src/auth.o \
       src/config/config.o \
       src/middleware/AuthMiddleware.o \
       src/services/DatabaseService.o \
       src/controllers/AuthController.o \
       src/controllers/ProviderController.o \
       src/controllers/ServiceController.o \
       src/controllers/AppointmentController.o \
       src/controllers/ReviewController.o \
       src/controllers/NotificationController.o \
       src/controllers/StatsController.o \
       src/sqlite3.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/sqlite3.o: src/sqlite3.c
	gcc -c -O2 -Iinclude -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	del /f src\main.o src\config\config.o src\middleware\AuthMiddleware.o src\services\DatabaseService.o src\controllers\*.o src\sqlite3.o $(TARGET)

run: $(TARGET)
	$(TARGET)