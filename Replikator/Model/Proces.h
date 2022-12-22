#pragma once
#include<stdio.h>
#include<winsock2.h>
#include"Poruka.h"
//struktura koja predstavlja proces i kako se on cuva
typedef struct proces {
	char ID;
	struct  Poruka* poruka;
	SOCKET acceptedSocket;
}PROCES;