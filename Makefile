all: parser
#target = parser

CFLAGS = -I/usr/include/mysql++ -I/usr/include/mysql -lmysqlpp -lmysqlclient -O3 -std=c++11 -Werror
CC = g++

parser: parse.cpp
	$(CC) parse.cpp -o parser $(CFLAGS)
