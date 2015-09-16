all: parser
#target = parser

CFLAGS = -I/usr/include/mysql++ -I/usr/include/mysql -lmysqlpp -lmysqlclient
CC = g++

parser: parse.cpp
	$(CC) parse.cpp -o parser $(CFLAGS)
