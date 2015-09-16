all: parser
#target = parser

CFLAGS = -I/usr/include/mysql++ -I/usr/include/mysql -lmysqlpp -lmysqlclient
CC = g++

parser:
	$(CC) parse.cpp -o parser $(CFLAGS)
