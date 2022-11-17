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
int Retval;
FILE* Fid_read,*Fid_write;
char* frame_recv = (char*)malloc(sizeof(char) * 1600);
fram* fr;
char* q_result = (char*)malloc(sizeof(char) * 4);
struct chap* chap1;
struct file* file1;

struct resend {
	char data_resend[FILE_MESS_MAX];
	int len;
};
struct resend* resend1 = (struct resend*)malloc(sizeof(resend));
int l = 0;//��¼����֡�����ݳ���

char recvfilesequence = 0;

int a = 0;

void Init();
void End();
int chap(SOCKET new_s,char* query_result);
char* chap_send(SOCKET new_s);
void S0_fsm();
void S1_fsm();
void End_fsm();
int SendFile(int sequence);
void RsendFile(int sequence);
void ready_fsm();
void R0_fsm();
void R1_fsm();
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
	new_s = accept(mainsock, (SOCKADDR*)&new_addr, &szClintAddr);//�������ӣ�������ѯ
	//if (new_s > 0) {
	//	status = READY;
	//	//status = UNCHAPED;
	//	//������ѯ����,���㲢���������
	//	q_result = chap_send(new_s);
	//}

	status = READY;//jump to status READY
	fd_set read_list;
	timeval timeout;

	while (1) {
		//to do: ���׽��ֺ��׽Ӷ��й������׽���һֱ������ʵ��һ�Զ�

		//�鿴״̬
		FD_ZERO(&read_list);
		FD_SET(new_s, &read_list);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		Retval = select(0, &read_list, NULL, NULL, &timeout);
		if (Retval == SOCKET_ERROR)
			break;
		if (Retval == 0) {
			event = EVENT_TIMEOUT;//timeout
		}else {
			if (!FD_ISSET(new_s, &read_list)) {
				continue;
			}
			Retval = recv(new_s, frame_recv, 1600, 0);
			printf("I recv %d byte,", Retval);
			if (Retval <= 0) {
				Retval = WSAGetLastError();
				if (Retval == WSAEWOULDBLOCK)
					continue;
				printf("Connection was closed");
				End();//���ӶϿ����ͷ��׽��֣��������ȴ��µ�����
				return 0;
			}
			fr = (fram*)frame_recv;
			//�ж�֡���ͣ������¼�
			if(fr->pro == CHAP){
				chap1 = (struct chap*)fr->MESS;
				event = EVENT_RECVCHAPANS;
			}
			else if(fr->pro == FILE_OPT){
				file1 = (struct file*)fr->MESS;
				if (file1->type == DATA) {
					if (file1->sequence == 0) event = EVENT_RECV0;
					else if (file1->sequence == 1) event = EVENT_RECV1;
				}
				else if (file1->type == ACK) {
					if (file1->sequence == 0) event = EVENT_RECVACK0;
					else if (file1->sequence == 1) event = EVENT_RECVACK1;
				}
				else if (file1->type == COMMAND) {
					if (file1->sequence == 0) event = EVENT_RECCOMMAND_DOWNLOAD;
					else if (file1->sequence == 1) event = EVENT_RECCOMMAND_CHECKDIR;
				}
			}
			else if (fr->pro == CLOSE) {}
			else if (fr->pro == ERR) {}
			else {}
		}
		//����
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
			case SEND0:
				S0_fsm();
				break;
			case SEND1:
				S1_fsm();
				break;
			case SENDEND:
				End_fsm();
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
			// ���յ�֡0 д���ļ�
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//����ACK
		SendACK(0);
		status = RECV0;
		break;
	}
	case EVENT_RECV1: {
		if (file1->filesequence == recvfilesequence) {
			// ���յ�֡1 д���ļ�
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//����ACK
		SendACK(1);
		status = RECV1;
		break;
	}
	case EVENT_RECCOMMAND_DOWNLOAD: {
		//���յ�����������������ļ���0����
		//TODO:����mess�е��ļ����������ļ�
		printf("Recv Command Dowlaod\n");
		l = SendFile(0);
		if (l < FILE_MESS_MAX) {
			status = SENDEND;
		}
		else {
			status = SEND0;
		}
		break;
	}
	case EVENT_RECCOMMAND_CHECKDIR: {
		//���յ�����Ŀ¼����鿴�ļ�Ŀ¼������
		printf("Recv Command CHECKDIR\n");
		status = READY;
		break;
	}
	default://Ҳ���������������������
		break;
	}
}

void R0_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//���յ�֡0 ��д���ļ�
		//����ACK
		SendACK(0);
		status = RECV0;
		break;
	case EVENT_RECV1:
		//���յ�֡1 д���ļ�
		if (file1->filesequence == recvfilesequence) {
			// ���յ�֡1 д���ļ�
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//����ACK
		SendACK(1);
		status = RECV1;
		break;
	}
}

void R1_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//���յ�֡0 д���ļ�
		if (file1->filesequence == recvfilesequence) {
			// ���յ�֡1 д���ļ�
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//����ACK
		SendACK(0);
		status = RECV0;
		break;
	case EVENT_RECV1:
		//���յ�֡1 д���ļ�
		//����ACK
		SendACK(1);
		status = RECV1;
		break;
	}
}

void SendACK(int towhichframe) {
	char* ack_message = creat_file_message(ACK, towhichframe, 0, 0, NULL);
	fram* fra_ack = createframe(FILE_OPT, 7, ack_message);
	send(new_s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	printf("Recv frame %d,Send ACK!\n", towhichframe);
	return;
}

//������Init()
//����: ��ʼ�����׽��ֳ�ʼ���������������ļ�������ʼ��
void Init() {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	srand((unsigned)time(NULL));
	fopen_s(&Fid_read, "E:\\2022full_netprogram_djs\\djs_project3\\server\\server_snd.txt", "rb");
	if (Fid_read == NULL) {
		printf("can`t open file\n");
		return;
	}
	fopen_s(&Fid_write, "E:\\2022full_netprogram_djs\\djs_project3\\server\\server_rx.txt", "wb");
	if (Fid_write == NULL) {
		printf("can`t open file\n");
		return;
	}

}

//������End()
//����: �ر��׽��֣��ر��ļ�����
void End() {
	
}

//������chap()
//����: ����chap��Ӧ/��ѯ���ģ�����/������ѯ���
int chap(SOCKET new_s, char* query_result) {
	//
	// ����Ӧ������ż��,Э����,type���
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
		/***��֪�ͻ�����ѯ���***/
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

//������chap_send()
//����: ������ѯ��Ӧ���ģ�������ѯ���
char* chap_send(SOCKET new_s) {
	/****��ѯ�׶�***/
	char query_result[4];
	struct chap chap0;
	int  chap1_mess_len = creat_chap_query_mess(&chap0, 10);
	//��ǰ��������
	get_query_result((unsigned char*)chap0.mess, key, query_result);
	//���ɴ�����֡������
	fram* fra = createframe(CHAP, chap1_mess_len + 2, (char*)(&chap0));
	//get_query_result((unsigned char*)((struct chap*)(fra->MESS))->mess, key, query_result);
	send(new_s, (char*)fra, get_frame_len(fra), 0);
	return query_result;
}

void End_fsm()
{
	switch (event) {
	case EVENT_RECVACK0: {
		//���յ����һ֡ ֡0��ACK���������
		printf("Recv ACK of Thelastframe.End\n");
		status = END;
		closesocket(new_s);
		break;
	}
	case EVENT_RECVACK1: {
		// ���յ����һ֡ ֡1��ACK���������
		printf("Recv ACK of Thelastframe.End\n");
		status = END;
		closesocket(new_s);
		break;
	}
	case EVENT_TIMEOUT:
		//��ʱ���ط���һ֡
		RsendFile(10);
		status = SENDEND;
		break;
	}
}

void S0_fsm()
{
	switch (event) {
	case EVENT_RECVACK0: {
		//���յ�֡0��ACK��������һ֡ ֡1
		printf("Recv ACK of The_0_frame.\n");
		l = SendFile(1);
		if (l < FILE_MESS_MAX) {
			status = SENDEND;
		}
		else {
			status = SEND1;
		}
		break;
	}
	case EVENT_TIMEOUT:
		//��ʱ���ط�֡0
		RsendFile(0);
		status = SEND0;
		break;
	}
}

void S1_fsm()
{
	switch (event) {
	case EVENT_RECVACK1: {
		//���յ�֡1��ACK��������һ֡ ֡0
		printf("Recv ACK of The_1_frame.\n");
		l = SendFile(0);
		if (l < FILE_MESS_MAX) {
			status = SENDEND;
		}
		else {
			status = SEND0;
		}
		break;
	}
	case EVENT_TIMEOUT:
		//��ʱ���ط�֡1
		RsendFile(1);
		status = SEND1;
		break;
	}
}

//����:int SendFile()
//���ܣ�����FILE_MESS_MAX���ζ�ȡ�ļ�������֡x(sequence)
int SendFile(int sequence) {
	char data[FILE_MESS_MAX];
	int len = fread(data, sizeof(char), sizeof(data), Fid_read);//ʵ�ʶ�ȡ����len��С,����len=FILE_MESS_MAX
	for (int i = 0; i < FILE_MESS_MAX; i++) {
		resend1->data_resend[i] = data[i];
	}
	resend1->len = len;//
	char* file_message = creat_file_message(DATA, sequence, 0, len, data);//filesequence����
	fram* fra_filedata = createframe(FILE_OPT, 7 + len, file_message);
	send(new_s, (char*)fra_filedata, get_frame_len(fra_filedata), 0);
	printf("Send frame %d,send %d byte.\n\n", sequence, get_frame_len(fra_filedata));
	return len;
}

//����:void RsendFile()
//���ܣ�resend1������һ�η��͵����ݺ����ݳ��ȣ����·���֡x(sequence)
void RsendFile(int sequence) {
	int len = resend1->len;
	char* file_message = creat_file_message(DATA, sequence, 0, len, resend1->data_resend);//����filesequence����
	fram* fra_filedata = createframe(FILE_OPT, 7 + len, file_message);
	send(new_s, (char*)fra_filedata, get_frame_len(fra_filedata), 0);
	if (sequence > 2)
		printf("Time out!Rsend Thelast frame,send %d byte.\n\n", get_frame_len(fra_filedata));
	else
		printf("Time out!Rsend frame %d,send %d byte.\n\n", sequence, get_frame_len(fra_filedata));
	return;
}