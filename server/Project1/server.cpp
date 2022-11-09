#include<stdio.h>
#include<winsock.h>
#include<string.h>
#pragma comment(lib, "WS2_32")
#include"frame.h"
#include"connect_control.h"
#include<stdlib.h>
int key= 12345;
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
	int x = 10;
	while (x--) {
		switch (status) {
		case unconnect: {
			new_s = accept(s, (SOCKADDR*)&new_addr, &szClntAddr);
			/****质询阶段***/
			struct chap chap0;
			int  chap1_mess_len = creat_chap_query_mess(&chap0, 10);
			//提前计算出结果
			char query_result[4];
			get_query_result((unsigned char*)chap0.mess, key, query_result);
			fram* fra = initframe(CHAP, chap1_mess_len + 2, (char*)(&chap0));
			//get_query_result((unsigned char*)((struct chap*)(fra->MESS))->mess, key, query_result);
			send(new_s, (char*)fra, get_frame_len(fra), 0);
			/****接受回应报文***/
			char* chap_ack = (char*)malloc(sizeof(char) * 1500);
			recv(new_s, chap_ack, 1500, 0);
			/***结果验证****/
			fram* fr = (fram*)chap_ack;
			struct chap* chap1 = (struct chap*)fr->MESS;
			//
			// 这里应该有序号检查,协议检查,type检查
			//
			int success = strncmp(chap1->mess, query_result, 4) == 0 ? 1 : 0;
			/***告知客户机质询结果***/
			char* chap_result = creat_chap_result_mess(success);
			fram* fra_resut = initframe(CHAP, 3, chap_result);
			send(new_s, (char*)fra_resut, get_frame_len(fra_resut), 0);
			if (success) status = connected;
			else { printf("chap failed"); closesocket(new_s); }
		}break;
		case connected:
			printf("chap success\n");
			break;
		default:break;
		}
	}
	system("pause");
	return 0;
}