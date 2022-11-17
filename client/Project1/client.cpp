#include<stdio.h>
#include"frame.h"
#include"file_opt.h"
#include<string.h>
#include<time.h>

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif
struct timeval tim;
int main() {
	tim.tv_sec = 1;
	tim.tv_usec = 0;
	//windows����socket����
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!\n");

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	printf("SOCKET%d", s);
	//SOCKET new_s;
	struct sockaddr_in saddr, new_addr;

	srand((unsigned)time(NULL));

	fd_set readfds, writefds, exceptfds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(1234);

	if (connect(s, (SOCKADDR*)&saddr, sizeof(saddr)) == -1) {
		printf("connect feiled\n");
		return 0;
	}
	//������ѯ����
	char* message = (char*)malloc(sizeof(char) * 1500);
	recv(s, message, 1500, 0);
	printf("�յ�CHAP��ѯ\n");
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
	fram* fra_ack = initframe(CHAP, strlen(ack_mess), ack_mess);
	//����ack����
	send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	printf("����ACK��Ӧ\n");
	//���ձ��Ĺ۲���ѯ�Ƿ�ɹ�
	recv(s, message, 1500, 0);
	fra = (fram*)message;
	chap1 = (struct chap*)fra->MESS;
	int chap_success = chap1->mess[0];
	if (chap_success == 1)printf("chap success");
	else printf("chap failed");
	//״̬�����̱���Ԥ����
	enum status stat = wait_cmd;
	struct file_opt opt; 
	struct fil file;
	int val;
	int cmd_finish;
	int finish;
	while(chap_success) {
		if (resend_time == 4)
			stat = clo;
		switch (stat) {
			case wait_cmd:
				opt.num = 0;
				//�Ӽ��̻�ȡ����
				printf("\n>>>");
				char str[200];
				fgets(str, 200, stdin);
				char* p;
				p = strtok(str, " ");
				while (p)
				{
					if (opt.num < 2)strcpy(opt.opt[opt.num], p);
					opt.num++;
					//printf("%s\n", p);
					p = strtok(NULL, " ");
				}
				//������Ҫһ��cmd�жϺ��������ж�����������Ƿ�Ϸ�...
				if (opt_check(opt, &file))stat = deal_cmd;
				else { printf("����������������������\n"); }
				break;
			case deal_cmd:
				//����cmd�������ָ�����ͱ���
				if (file.type == SHOW) { fra = initframe(FILE_OPT, 102, (char*)(&file)); stat = recv_file; }
				else if(file.type == DOWNLOAD) { fra = initframe(FILE_OPT, 12 + FIEENAME_MAXLEN, (char*)(&file)); stat = recv_file;}
				else { fra = initframe(FILE_OPT, 2 + FIEENAME_MAXLEN, (char*)(&file)); stat = send_file; }
				send(s, (char*)fra, get_frame_len(fra), 0);
				FD_SET(s, &readfds);
				tim.tv_sec = 1;
				val = select(0, &readfds, &writefds, &exceptfds, &tim);
				if (val == 0) { stat = deal_cmd; resend_time++; printf("i resend\n"); }
				else { 
					resend_time = 0; 
					recv(s, message, 1500, 0);
					ack_ckeck(message, packet_seq);
					if (file.type == UPLOAD) packet_seq++;
				}
				break;
			case send_file:
				printf("packet_seq=%d\n", packet_seq);
				finish = send_file_mess(s, file, packet_seq);
				FD_SET(s, &readfds);
				tim.tv_sec = 1;
				val = select(0, &readfds, &writefds, &exceptfds, &tim);
				if (val == 0) { stat = send_file; resend_time++; }
				else { 
					recv(s, message, 1500, 0);
					ack_ckeck(message, packet_seq);
					resend_time = 0; packet_seq++; packet_seq %= 255; if (finish) { fclose(ffile); stat = wait_cmd; }
				}
				break;
			case recv_file:
				//�����ļ�����
				FD_SET(s, &readfds);
				tim.tv_sec = 3;
				val = select(0, &readfds, &writefds, &exceptfds, &tim);
				if (val == 0)  stat = clo;
				else {
					recv(s, message, 1500, 0);
					cmd_finish = deal_recv_file(s,message, &packet_seq);
					if (cmd_finish) stat = finish_cmd;
				}
				break;
			case finish_cmd:
				printf("finish\n");
				FD_SET(s, &readfds);
				tim.tv_sec = 2;
				val = select(0, &readfds, &writefds, &exceptfds, &tim);
				if (val == 0) { stat = wait_cmd; printf("cmd finish\n"); }
				else {
					recv(s, message, 1500, 0);
					cmd_finish = deal_recv_file(s,message, &packet_seq);
					if (cmd_finish) stat = finish_cmd;
				}
				break;
			case clo:
				chap_success = 0;
				break;
		}
	}
	system("pause");
	return 0;
}

/*�����ƣ��ٶ��ڱ��Ķ�ʧ�Ŀ��ǣ�
һ���Ƿ����ļ����ݶ�ʧ������ack��ʱ�ش���
������ack��ʧ�����Ͷ��ش����յ��ظ�����,��Ҫ���շ����Ӹ������ظ������ƣ��ظ����Ĳ����ж�д
�����Ƕ���Ҫ����ack
���ڶ�ȡָ��ͽ��ձ�����������Ŀ��ǣ�
��������һ��ָ�����ʱ����������ָ�������һ��ָ���������ack��������ʱ���Է��ط�����
�����Ҫ�ڽ��յ�ָ���2*timeout��Ҫ������һ��ָ�����һ�������Ƿ��յ�ack,
���������һ��ack���ͺ���3*timeout��ʱ�䣬�ڽ���������ܽ׶�
�ڶ��������������ķ��ͺ�û���յ�ack�������϶�Ϊ��·������̫�ߣ������Ͽ��ļ����͹���
���շ����������������Ķ���ʧ�ˣ�Ҳ�����Ƕ��˼���ack,
��Ҫһ���ж����̣��ļ���Ƭ�ˣ�����һ��ʱ����δ�յ��������ģ�Ҳ��Ҫһ����ʱ����
��3*timeout
*/


/*
�ܽ᣺
֡��ʧ����ָ���Լ��ļ�����ack�׶�(��CHAP����ο���ϸ��״̬������Ҳ���ӱ��Ķ�ʧ����)
���б��Ķ������ش������������ú�ʱ���Ϊ����
���ͱ��ĺ��ļ����ݻ�ָ��ģ���ʱʱ��Ϊtimeout���������ش�������Ҫ�ش�������ʱ�����Ͽ�����
ack�������ó�ʱʱ��Ϊ3.5*timeout,��ʱ�Ͽ����ӣ������Ϊ���ֶϿ����ӣ�һ�����������������Ͽ����ͣ���һ�������ָ����һ��ack����

*/