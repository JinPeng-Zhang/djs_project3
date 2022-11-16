#include<stdio.h>
#include<winsock.h>
#include<string.h>
#include<stdlib.h>
#include"frame.h"
#include"connect_control.h"
#pragma comment(lib, "WS2_32")

int key = 12345;
SOCKET mainsock, new_s;
WSADATA wsaData;
FILE* Fid, * LogFid;
int Retval;
char* frame_recv = (char*)malloc(sizeof(char) * 1500);
fram* fr;
char* q_result = (char*)malloc(sizeof(char) * 4);
struct chap* chap1;
struct file* file1;
char recvfilesequence = 0;

int a = 0;

void ready_fsm();
void R0_fsm();
void R1_fsm();
void Init();
void End();
int chap(SOCKET new_s,char* query_result);
char* chap_send(SOCKET new_s);
void SendACK(int towhichframe);

int main() {
	Init();
	mainsock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr, new_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(0x1234);

	Retval = bind(mainsock, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (listen(mainsock, 5) == SOCKET_ERROR)
		printf("error");
	int szClintAddr = sizeof(new_addr);
	new_s = accept(mainsock, (SOCKADDR*)&new_addr, &szClintAddr);//接收连接，发送质询
	if (new_s > 0) {
		status = READY;
		//status = UNCHAPED;
		//发送质询报文,计算并储存计算结果
		q_result = chap_send(new_s);
	}

	fd_set read_list;
	timeval timeout;

	while (1) {
		//查看状态
		FD_ZERO(&read_list);
		FD_SET(new_s, &read_list);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		Retval = select(0, &read_list, NULL, NULL, &timeout);
		if (Retval == SOCKET_ERROR)
			break;//what is this?
		if (Retval == 0) {
			event = EVENT_TIMEOUT;//timeout
		}else {
			if (!FD_ISSET(new_s, &read_list)) {
				continue;
			}
			Retval = recv(new_s, frame_recv, 1500, 0);
			if (Retval <= 0) {
				Retval = WSAGetLastError();
				if (Retval == WSAEWOULDBLOCK)
					continue;
				printf("Connection was closed");
				End();//连接断开，释放套接字，服务器等待新的连接
				return 0;
			}
			fr = (fram*)frame_recv;
			//判断帧类型，触发事件
			if(fr->pro == CHAP){
				chap1 = (struct chap*)fr->MESS;
				event = EVENT_RECVCHAPANS;
			}
			if(fr->pro == FILE_OPT){
				file1 = (struct file*)fr->MESS;
				if (file1->type == DATA) {
					if (file1->sequence == 0) event = EVENT_RECV0;
					if (file1->sequence == 1) event = EVENT_RECV1;
				}
				/*if (file1->type == ACK) {
					if (file1->sequence == 0) event = EVENT_RECVACK0;
					if (file1->sequence == 1) event = EVENT_RECVACK1;
				}*/
			}
			if (fr->pro == CLOSE) {}
			if (fr->pro == ERR) {}
		}
		//动作
		switch (status) {
			case UNCHAPED:
				chap(new_s, q_result);
				break;
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
	return 0;
}

void ready_fsm()
{
	switch (event) {
	case EVENT_RECV0: {
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧0 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid);
		}
		//返回ACK
		if (a > 0) SendACK(0);
		status = RECV0;
		break;
	}
	case EVENT_RECV1: {
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧1 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid);
		}
		//返回ACK
		SendACK(1);
		status = RECV1;
		break;
	}
	default://也可能是其他情况，待补充

		break;
	}
}

void R0_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 不写入文件
		//返回ACK
		SendACK(0);
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧1 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid);
		}
		//返回ACK
		//if (a > 0) {//模拟ACK丢失
		SendACK(1);
		
		status = RECV1;
		break;
	}
}

void R1_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 写入文件
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧1 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid);
		}
		//返回ACK
		SendACK(0);
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		//返回ACK
		SendACK(1);
		status = RECV1;
		break;
	}
}

void SendACK(int towhichframe) {
	char* ack_message = creat_file_message(ACK, towhichframe, 0, 0, NULL);
	fram* fra_ack = createframe(FILE_OPT, 8, ack_message);
	send(new_s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	printf("Recv frame %d,Send ACK!\n", towhichframe);
	return;
}

//函数：Init()
//功能: 初始化，套接字初始化，开启监听，文件函数初始化
void Init() {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	srand((unsigned)time(NULL));
	fopen_s(&Fid,"E:\\2022full_netprogram_djs\\djs_project3\\server\\rx.txt", "wb");
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
//功能: 处理chap回应/质询报文，返回/发送质询结果
int chap(SOCKET new_s, char* query_result) {
	//
	// 这里应该有序号检查,协议检查,type检查
	//
	int result = 0;
	switch (event) {
	case EVENT_RECVCHAPANS: {
		result = strncmp(chap1->mess, query_result, 4) == 0 ? 1 : 0;
		if (result)
		{
			printf("chap result\n");
			status = READY;
		}
		else
		{
			printf("chap failed\n");
			closesocket(new_s);
			status = UNCHAPED;
		}
		/***告知客户机质询结果***/
		char* chap_result = creat_chap_result_mess(result);
		fram* fra_resut = createframe(CHAP, 3, chap_result);
		send(new_s, (char*)fra_resut, get_frame_len(fra_resut), 0);
	}
	default:
		status = UNCHAPED;
		break;
	}
	return result;
}

//函数：chap_send()
//功能: 处理质询回应报文，返回质询结果
char* chap_send(SOCKET new_s) {
	/****质询阶段***/
	char query_result[4];
	struct chap chap0;
	int  chap1_mess_len = creat_chap_query_mess(&chap0, 10);
	//提前计算出结果
	get_query_result((unsigned char*)chap0.mess, key, query_result);
	//生成待发送帧，发送
	fram* fra = createframe(CHAP, chap1_mess_len + 2, (char*)(&chap0));
	//get_query_result((unsigned char*)((struct chap*)(fra->MESS))->mess, key, query_result);
	send(new_s, (char*)fra, get_frame_len(fra), 0);
	return query_result;
}
