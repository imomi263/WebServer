CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

TARGET = server
OBJS = ../code/log/m*.cpp ../code/pool/m*.cpp ../code/timer/m*.cpp \
		../code/http/m*.cpp ../code/server/m*.cpp \
       ../code/buffer/m*.cpp ../code/m_main.cpp


all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)