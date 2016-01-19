all: parser
#target = parser

CFLAGS = -I/usr/include/mysql++ -I/usr/include/mysql -lmysqlpp -lmysqlclient
CFLAGS += -std=c++11
CFLAGS += -Werror
CFLAGS += -O3
CC = g++

parser: parse.cpp
	$(CC) parse.cpp -o parser $(CFLAGS)
