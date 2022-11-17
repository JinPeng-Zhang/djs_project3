#include<stdio.h>
#include<winsock.h>
#include<string.h>
#pragma comment(lib, "WS2_32")
#include"frame.h"
#include"connect_control.h"
#include"file_opt.h"
#include<stdlib.h>
int key= 12345;
struct timeval tim;
int resend_time = 0;
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

	fd_set readfds, writefds, exceptfds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(1234);

	int b = bind(s, (struct sockaddr*)&saddr, sizeof(saddr));
	if (listen(s, 5) == SOCKET_ERROR)
		printf("error");
	char* message = (char*)malloc(sizeof(char) * 1500);
	int szClntAddr = sizeof(new_addr);
	int success,val,finish,cmd_finish;
	struct fil fil;
	enum status stat = wait_cmd;
	while (1) {
		switch (status) {
		case unconnect: {
			new_s = accept(s, (SOCKADDR*)&new_addr, &szClntAddr);
			printf("connect to client\n");
			/****质询阶段***/
			struct chap chap0;
			int  chap1_mess_len = creat_chap_query_mess(&chap0, 10);
			//提前计算出结果
			char query_result[4];
			get_query_result((unsigned char*)chap0.mess, key, query_result);
			fram* fra = initframe(CHAP, chap1_mess_len + 2, (char*)(&chap0));
			//get_query_result((unsigned char*)((struct chap*)(fra->MESS))->mess, key, query_result);
			send(new_s, (char*)fra, get_frame_len(fra), 0);
			printf("发送质询报文\n");
			/****接受回应报文***/
			char* chap_ack = (char*)malloc(sizeof(char) * 1500);
			recv(new_s, chap_ack, 1500, 0);
			printf("收到回应报文\n");
			/***结果验证****/
			fram* fr = (fram*)chap_ack;
			struct chap* chap1 = (struct chap*)fr->MESS;
			//
			// 这里应该有序号检查,协议检查,type检查
			//
			success = strncmp(chap1->mess, query_result, 4) == 0 ? 1 : 0;
			/***告知客户机质询结果***/
			char* chap_result = creat_chap_result_mess(success);
			fram* fra_resut = initframe(CHAP, 3, chap_result);
			send(new_s, (char*)fra_resut, get_frame_len(fra_resut), 0);
			if (success) { printf("chap success\n"); status = connected; }
			else { printf("chap failed"); closesocket(new_s); }
			stat = wait_cmd;
			resend_time = 0;
		}break;
		case connected:
			while (success) {
				if (resend_time == 4)
					stat = clo;
				switch (stat) {
					case wait_cmd:
						printf("wait cmd...\n");
						FD_SET(new_s, &readfds);
						tim.tv_sec = 60;
						val = select(0, &readfds, &writefds, &exceptfds, &tim);
						if (val == 0) { stat = clo; printf("no cmd for a long time\n"); }
						else { recv(new_s,message,1500,0); stat = deal_cmd(new_s,message,&fil);
						if (stat == recv_file) packet_seq--;
						}
						break;
					case send_file:
						finish = send_file_mess(new_s, fil, packet_seq);
						FD_SET(new_s, &readfds);
						tim.tv_sec = 1;
						val = select(0, &readfds, &writefds, &exceptfds, &tim);
						if (val == 0) { stat = send_file; resend_time++; }
						else { 
							recv(new_s, message, 1500, 0);
							ack_ckeck(message, packet_seq);
							resend_time = 0; packet_seq++; packet_seq %= 255; if (finish) { stat = wait_cmd; if(checkfile(fil.filename))fclose(ffile);
							}
						}
						break;
					case recv_file:
						//接收文件内容
						FD_SET(new_s, &readfds);
						tim.tv_sec = 3;
						val = select(0, &readfds, &writefds, &exceptfds, &tim);
						if (val == 0)  stat = clo;
						else {
							recv(new_s, message, 1500, 0);
							cmd_finish = deal_recv_file(new_s,message, &packet_seq);
							if (cmd_finish) stat = finish_cmd;
						}
						break;
					case finish_cmd:
						printf("finish\n");
						FD_SET(new_s, &readfds);
						tim.tv_sec = 1;
						val = select(0, &readfds, &writefds, &exceptfds, &tim);
						if (val == 0) { stat = wait_cmd; printf("cmd finish\n"); }
						else {
							recv(new_s, message, 1500, 0);
							cmd_finish = deal_recv_file(new_s,message, &packet_seq);
							if (cmd_finish) stat = finish_cmd;
						}
						break;
					case clo:
						success = 0;
						break;
				}
			}
			status = unconnect;
			break;
		default:break;
		}
	}
	system("pause");
	return 0;
}