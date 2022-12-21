#pragma once
#include<stdio.h>
#include<winsock2.h>
#include"Poruka.h"
typedef struct proces {
	char ID;
	struct  Poruka* poruka;
	SOCKET acceptedSocket;
}PROCES;