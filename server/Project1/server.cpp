#include<stdio.h>
#include<winsock.h>
#include<string.h>
#pragma comment(lib, "WS2_32")
#include"frame.h"
#include<stdlib.h>
int main() {
	//printf("%d", sizeof(char));
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	SOCKET new_s;
	struct sockaddr_in saddr, new_addr;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(1234);

	int b = bind(s, (struct sockaddr*)&saddr, sizeof(saddr));
	if (listen(s, 5) == SOCKET_ERROR)
		printf("error");
	int szClntAddr = sizeof(new_addr);
	//new_s = accept(s, (SOCKADDR*)&new_addr, &szClntAddr);
	fram* fra = initframe(ERR,0x00,NULL);
	char*  st = (char*)fra;
	/*
	printf("%#x\n", fra->pro);
	printf("%x\n", &(fra->version));
	printf("%x\n", &(fra->MESS_LEN));
	printf("%x\n", &(fra->MESS));
	printf("%d\n",strlen(st));
	for (int i = 0; i < strlen(st); i++)
		printf("%x %02hhx\n", st+i,*(st+i));
	printf("%d",get_frame_len(fra));
	*/
	//send(new_s, st, get_frame_len(fra),0);
	return 0;
}