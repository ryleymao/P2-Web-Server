#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 4096

void start_server(void (*handler)(char *, int), int port);

#endif