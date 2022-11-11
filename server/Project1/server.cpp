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

//������Init()
//����: ��ʼ�����׽��ֳ�ʼ���������������ļ�������ʼ��
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

//������End()
//����: �ر��׽��֣��ر��ļ�����
void End() {

}

//������chap()
//����: ��new_s����chap��ѯ���̣�������ѯ���
int chap(SOCKET new_s) {
	/****��ѯ�׶�***/
	struct chap chap0;
	int  chap1_mess_len = creat_chap_query_mess(&chap0, 10);
	//��ǰ��������
	char query_result[4];
	get_query_result((unsigned char*)chap0.mess, key, query_result);
	//���ɴ�����֡������
	fram* fra = initframe(CHAP, chap1_mess_len + 2, (char*)(&chap0));
	//get_query_result((unsigned char*)((struct chap*)(fra->MESS))->mess, key, query_result);
	send(new_s, (char*)fra, get_frame_len(fra), 0);
	/****���ܻ�Ӧ����***/
	recv(new_s, frame_recv, 1500, 0);
	/***�����֤****/
	fr = (fram*)frame_recv;
	struct chap* chap1 = (struct chap*)fr->MESS;
	//
	// ����Ӧ������ż��,Э����,type���
	//
	int success = strncmp(chap1->mess, query_result, 4) == 0 ? 1 : 0;
	/***��֪�ͻ�����ѯ���***/
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
		//�鿴״̬
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
			//�ж�֡���ͣ������¼�
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
		//����
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
		//���յ�֡0 д���ļ�
		//����ACK
		status = RECV0;
		break;
	case EVENT_RECV1:
		//���յ�֡1 д���ļ�
		//����ACK
		status = RECV1;
		break;
	}
}

void R0_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//���յ�֡0 ��д���ļ�
		//����ACK
		status = RECV0;
		break;
	case EVENT_RECV1:
		//���յ�֡1 д���ļ�
		//����ACK
		status = RECV1;
		break;
	}
}

void R1_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//���յ�֡0 д���ļ�
		//����ACK
		status = RECV0;
		break;
	case EVENT_RECV1:
		//���յ�֡1 д���ļ�
		//����ACK
		status = RECV1;
		break;
	}
}