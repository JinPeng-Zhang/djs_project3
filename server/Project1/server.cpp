#include<stdio.h>
#include<winsock.h>
#include<string.h>
#pragma comment(lib, "WS2_32")
#include"frame.h"
#include"connect_control.h"
#include<stdlib.h>
char key[] = "12345";
int main() {
	//printf("%d", sizeof(char));
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	//
	srand((unsigned)time(NULL));
	//
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
	switch (status) {
		case unconnect: {
			new_s = accept(s, (SOCKADDR*)&new_addr, &szClntAddr);
			/****质询阶段***/
			char* chap_mess = creat_chap_query_mess(10);
			char* query_result = get_query_result(chap_mess, key);
			fram* fra = initframe(CHAP, strlen(chap_mess), chap_mess);
			send(new_s, (char*)fra, get_frame_len(fra), 0);
			/****接受回应报文***/
			char* chap_ack = (char*)malloc(sizeof(char) * CHAP_MESS_MAX);
			recv(new_s, chap_ack, CHAP_MESS_MAX, 0);
			/***结果验证****/
			fram* fr = (fram*)chap_ack;
			int success = strncmp(fr->MESS, query_result, 5) == 0 ? 1 : 0;
			char* chap_result = creat_chap_result_mess(success);
			fram* fra_resut = initframe(CHAP, strlen(chap_result), chap_result);
			send(new_s, (char*)fra_resut, get_frame_len(fra_resut), 0);
			if (success) status = connected;
			else status = clos;
		}break;
		case connected:
			while (1) {
				//接受命令、解析命令
				//执行相应文件操作
			}
			break;
		case clos:
			closesocket(new_s);
			status = unconnect;
			break;
	}
	return 0;
}