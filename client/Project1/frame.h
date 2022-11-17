#pragma once
//frame.h��Ҫ����֡�ṹ�嶨��ʹ�����
#define FRAME_VERSION 0X01
#include<stdlib.h>
#include<time.h>
#include<math.h>
#define FRAME_MAX 1500
#define MESS_MAX 1494
#define CHAP_MESS_MAX MESS_MAX-2
#define FILE_MESS_MAX MESS_MAX-7
//#define MAX_INT_NUM 360
unsigned char  sequence;
extern char query_result[4];
int key = 12345;
enum protocol
{
	CHAP,
	FILE_OPT,
	CLOSE,
	ERR,
};

enum file_type
{
	DATA,
	ACK,
	COMMAND,//only 2 type: download/check file dir
};

typedef struct frame {
	unsigned char version = FRAME_VERSION;//�Ѿ�������ֵ,1 byte
	unsigned char pro;//1 byte
	int MESS_LEN;//4 bytes
	char MESS[MESS_MAX];
}fram;
/*
struct integer {
	int N;
	char len_seq[MAX_INT_NUM/8];
	char int_seq[MAX_INT_NUM];
};
*/

//֡����chap
struct chap {
	unsigned char type;
	unsigned char  sequence;
	char mess[CHAP_MESS_MAX];
};

struct file_opt
{
	char type;
	char filename[20];
	void* mess;
};

//֡����file
struct file {
	char type;//DATA or ACK ,1 byte
	char sequence;//0 or 1,1 byte
	char filesequence;//from 0 to ... ,1 byte
	int len;//len of data,4 byte
	char data[FILE_MESS_MAX];//FILE_MESS_MAX=1484 byte
};

struct clos
{
	char  sequence;
};
struct err
{
	char type;
};

//����:fram* createframe()
//����:���ɸ������͡���Ϣ���ݵĴ�����֡
fram* createframe(enum protocol pro, int  mess_len, char* mess) {
	fram fra;
	fra.pro = pro;
	fra.MESS_LEN = mess_len;//mess_len�����FILE_MESS_MAX+7 or CHAP_MESS_MAX+2
	for (int i = 0; i < mess_len; i++)
		fra.MESS[i] = mess[i];
	//fra.MESS[mess_len] = '\0';
	return &fra;
}

char* creat_file_message(int type, int sequence, int filesequence, int data_len, char* data) {
	struct file* f = (struct file*)malloc(sizeof(struct file));
	f->type = type;
	f->sequence = sequence;
	f->filesequence = 0;//��Ч
	if (data_len > FILE_MESS_MAX)
	{
		printf("data large than file_data[]\n");
		return NULL;//1484<=1484
	}
	f->len = data_len;
	for (int i = 0; i < data_len; i++) {
		f->data[i] = data[i];
	}
	//f->data[data_len] = '\0';
	return (char*)f;
}

void get_query_result(unsigned char* chap_mess, int key, char* query_result) {
	int N = chap_mess[0];
	int num = 0;
	unsigned char* seq_len = chap_mess + 1;
	unsigned char* seq = chap_mess + 1 + N / 8;
	for (int i = 0; i < N / 8; i++) {
		for (int j = 0; j < 8; j++) {
			int k = (seq_len[i] >> (7 - j)) & 0x01;
			if (k) { num += seq[0] << 24 + seq[1] << 16 + seq[2] << 8 + seq[3]; seq = seq + 4; }
			else { num += seq[0] << 8 + seq[1]; seq = seq + 2; }
			//����û�п���int������⣬��֪������᲻���Զ�ɾ�����
		}
	}
	num = num ^ key;
	for (int i = 0; i < 4; i++)
		query_result[i] = num >> i * 8;
	//return (char*)(&num);
}

char* creat_chap_ack_mess(char* query_result) {
	struct chap* chap1 = (struct chap*)malloc(sizeof(struct chap));
	chap1->type = 0x01;
	chap1->sequence = sequence;
	for (int i = 0; i < 4; i++)
		chap1->mess[i] = query_result[i];
	chap1->mess[4] = '\0';
	return (char*)chap1;
}
int get_frame_len(fram* fra) {
	//return ((char*)(fra->MESS) - (char*)fra) + fra->MESS_LEN;
	return 6 + fra->MESS_LEN;
}
