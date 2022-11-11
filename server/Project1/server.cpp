#include<stdio.h>
#include<winsock.h>
#include<string.h>
#include<stdlib.h>
#include"frame.h"
#include"connect_control.h"
#pragma comment(lib, "WS2_32")

int key = 12345;
SOCKET s, new_s;
WSADATA wsaData;
FILE* Fid, * LogFid;
int Retval;
char* frame_recv = (char*)malloc(sizeof(char) * 1500);
fram* fr;

void ready_fsm();
void R0_fsm();
void R1_fsm();
void Init();
int chap(SOCKET new_s);

//函数：Init()
//功能: 初始化，套接字初始化，开启监听，文件函数初始化
void Init() {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	srand((unsigned)time(NULL));
	Fid = fopen("rx.txt", "wb");
	if (Fid == NULL) {
		printf("can`t open file\n");
		return;
	}

}

//函数：End()
//功能: 关闭套接字，关闭文件函数
void End() {

}

//函数：chap()
//功能: 与new_s进行chap质询过程，返回质询结果
int chap(SOCKET new_s) {
	/****质询阶段***/
	struct chap chap0;
	int  chap1_mess_len = creat_chap_query_mess(&chap0, 10);
	//提前计算出结果
	char query_result[4];
	get_query_result((unsigned char*)chap0.mess, key, query_result);
	//生成待发送帧，发送
	fram* fra = initframe(CHAP, chap1_mess_len + 2, (char*)(&chap0));
	//get_query_result((unsigned char*)((struct chap*)(fra->MESS))->mess, key, query_result);
	send(new_s, (char*)fra, get_frame_len(fra), 0);
	/****接受回应报文***/
	recv(new_s, frame_recv, 1500, 0);
	/***结果验证****/
	fr = (fram*)frame_recv;
	struct chap* chap1 = (struct chap*)fr->MESS;
	//
	// 这里应该有序号检查,协议检查,type检查
	//
	int success = strncmp(chap1->mess, query_result, 4) == 0 ? 1 : 0;
	/***告知客户机质询结果***/
	char* chap_result = creat_chap_result_mess(success);
	fram* fra_resut = initframe(CHAP, 3, chap_result);
	send(new_s, (char*)fra_resut, get_frame_len(fra_resut), 0);
	return success;
}


int main() {

	Init();
	s = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr, new_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(0x1234);

	Retval = bind(s, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (listen(s, 5) == SOCKET_ERROR)
		printf("error");
	int szClintAddr = sizeof(new_addr);
	new_s = accept(s, (SOCKADDR*)&new_addr, &szClintAddr);

	fd_set read_list;
	timeval timeout;

	int x = 10;
	while (x--) {
		//查看状态
		FD_ZERO(&read_list);
		FD_SET(new_s, &read_list);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		Retval = select(0, &read_list, NULL, NULL, &timeout);
		if (Retval == SOCKET_ERROR)
			break;//what is this?
		if (Retval == 0) {
			event = EVENT_TIMEOUT;
		}else {
			if (!FD_ISSET(new_s, &read_list)) {
				continue;
			}
			//Retval = recv(new_s, frame_recv, 1500, 0);
			if (Retval <= 0) {
				Retval = WSAGetLastError();
				if (Retval == WSAEWOULDBLOCK)
					continue;
				printf("Connection was closed");
				End();
			}
			//fr = (fram*)frame_recv;
			//判断帧类型，触发事件
			if(fr->pro == CHAP){}
			if(fr->pro == FILE_OPT){
				struct file* file1 = (struct file*)fr->MESS;
				if (file1->type == DATA) {
					if (file1->sequence == 0) event = EVENT_RECV0;
					if (file1->sequence == 1) event = EVENT_RECV1;
				}
				/*if (file1->type == ACK) {
					event = EVENT_RECVACK;
				}*/
			}
		}
		//动作
		switch (status) {
			case UNCHAPED: {
				int result = chap(new_s);
				if (result)
				{
					printf("chap success\n");
					status = READY;
				}
				else 
				{ 
					printf("chap failed\n"); 
					closesocket(new_s); 
					status = READY;
				}
				}break;
			case READY:
				ready_fsm();
				break;
			case RECV0:
				R0_fsm();
				break;
			case RECV1:
				R1_fsm();
				break;
			default:break;
		}
	}
	system("pause");
	return 0;
}

void ready_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 写入文件
		//返回ACK
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		//返回ACK
		status = RECV1;
		break;
	}
}

void R0_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 不写入文件
		//返回ACK
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		//返回ACK
		status = RECV1;
		break;
	}
}

void R1_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 写入文件
		//返回ACK
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		//返回ACK
		status = RECV1;
		break;
	}
}