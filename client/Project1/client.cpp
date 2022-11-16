#include<stdio.h>
#include<winsock.h>
#pragma comment(lib, "WS2_32")
#include"frame.h"
#include"connect_control.h"

WSADATA wsaData;
SOCKET s;
FILE* Fid;
int Retval;
char* frame_recv = (char*)malloc(sizeof(char) * 1500);
fram* fr;
struct chap* chap1;
struct file* file1;
struct resend {
	char data_resend[FILE_MESS_MAX];
	int len;
};
struct resend* resend1= (struct resend*)malloc(sizeof(resend));

int chap();
void Init();
void End();
void S0_fsm();
void S1_fsm(); 
void End_fsm();
int SendFile(int sequence);
void RsendFile(int sequence);

int main() {
	Init();
	s = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in saddr, new_addr;


	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(0x1234);

	if (connect(s, (SOCKADDR*)&saddr, sizeof(saddr)) == -1) {
		printf("connect failed\n");
		return 0;
	}

	fd_set read_list;
	timeval timeout;
	unsigned long arg;
	arg = 1;
	ioctlsocket(s, FIONBIO, &arg);
	//����֡0
	SendFile(0);
	status = SEND0;

	while (1) {
		FD_ZERO(&read_list);
		FD_SET(s, &read_list);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		Retval = select(0, &read_list, NULL, NULL, &timeout);
		if (Retval == SOCKET_ERROR)
			break;
		if (Retval == 0) {
			event = EVENT_TIMEOUT;
		}
		else {
			if (!FD_ISSET(s, &read_list)) {
				continue;
			}
			Retval = recv(s, frame_recv, 1500, 0);
			if (Retval <= 0) {
				Retval = WSAGetLastError();
				if (Retval == WSAEWOULDBLOCK)
					continue;
				End();
			}//��⵽�Ͽ�����
			fr = (fram*)frame_recv;
			if (fr->pro == FILE_OPT) {
				file1 = (struct file*)fr->MESS;
				if (file1->type == DATA) {
					if (file1->sequence == 0) event = EVENT_RECV0;
					else if (file1->sequence == 1) event = EVENT_RECV1;
				}
				else if (file1->type == ACK) {
					if (file1->sequence == 0) event = EVENT_RECVACK0;
					else if (file1->sequence == 1) event = EVENT_RECVACK1;
				}
			}
			else if (fr->pro == CHAP) {
				//��֤״̬
			}
		}
		switch (status) {
		case SEND0:
			S0_fsm();
			break;
		case SEND1:
			S1_fsm();
			break;
		case SENDEND:
			End_fsm();
			break;
		default:
			End();
		}
	}


	return 0;
}

int chap() {
	//������ѯ����
	char* message = (char*)malloc(sizeof(char) * 1500);
	recv(s, message, 1500, 0);
	//���
	fram* fra = (fram*)message;
	struct chap* chap1 = (struct chap*)fra->MESS;
	//��������
	sequence = chap1->sequence;
	//�������
	char query_result[4];
	get_query_result((unsigned char*)chap1->mess, key, query_result);
	//����ack����
	char* ack_mess = creat_chap_ack_mess(query_result);
	fram* fra_ack = createframe(CHAP, strlen(ack_mess), ack_mess);
	//����ack����
	send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	//���ձ��Ĺ۲���ѯ�Ƿ�ɹ�
	recv(s, message, 1500, 0);
	fra = (fram*)message;
	chap1 = (struct chap*)fra->MESS;
	int chap_success = chap1->mess[0];
	if (chap_success == 1)printf("chap success");
	else printf("chap failed");
	return chap_success;
}

//������Init()
//����: ��ʼ�����׽��ֳ�ʼ���������������ļ�������ʼ��
void Init() {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	
	fopen_s(&Fid,"E:\\2022full_netprogram_djs\\djs_project3\\client\\snd.txt", "rb");
	if (Fid == NULL) {
		printf("can`t open file\n");
		return;
	}

}

//������End()
//����: �ر��׽��֣��ر��ļ�����
void End() {
	closesocket(s);
	fclose(Fid);
	WSACleanup();
}

void End_fsm()
{
	switch (event) {
	case EVENT_RECVACK0: {
		//���յ����һ֡ ֡0��ACK���������
		status = END;
		closesocket(s);
		break;
	}
	case EVENT_RECVACK1: {
		// ���յ����һ֡ ֡1��ACK���������
		status = END;
		closesocket(s);
		break;
	}
	case EVENT_TIMEOUT:
		//��ʱ���ط���һ֡
		/*printf("%d\n", frame_send->pro);
		printf("%d\n", frame_send->MESS_LEN);
		file* ff = (struct file*)frame_send->MESS;
		printf("%d\n", ff->type);
		printf("%d\n", ff->len);
		for(int i=0;i<ff->len;i++)
		printf("%d\n", ff->data[i]);
		*/
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
		int l = SendFile(1);
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
		int l = SendFile(0);
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

int SendFile(int sequence) {
	char data[FILE_MESS_MAX];
	int len = fread(data, sizeof(char), sizeof(data), Fid);//ʵ�ʶ�ȡ����len��С
	for (int i = 0; i < FILE_MESS_MAX; i++) {
		resend1->data_resend[i] = data[i];
	}
	resend1->len = len;
	char* file_message = creat_file_message(DATA, sequence, 0, len, data);//filesequence����
	fram* fra_filedata = createframe(FILE_OPT, 7 + len, file_message);
	send(s, (char*)fra_filedata, get_frame_len(fra_filedata), 0);
	printf("Send frame %d\n", sequence);
	return len;
}
void RsendFile(int sequence) {
	int len = resend1->len;
	char* file_message = creat_file_message(DATA, sequence, 0, len, resend1->data_resend);//filesequence����
	fram* fra_filedata = createframe(FILE_OPT, 7 + len, file_message);
	send(s, (char*)fra_filedata, get_frame_len(fra_filedata), 0);
	if (sequence > 2)
		printf("Time out!Rsend last frame.\n");
	else
		printf("Time out!Rsend frame %d\n", sequence);
	return;
}