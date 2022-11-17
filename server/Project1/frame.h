#pragma once
//frame.h需要包含帧结构体定义和处理函数
#define FRAME_VERSION 0X01
#include<stdlib.h>
#include<time.h>
#include<math.h>
#define MESS_MAX 1494
#define CHAP_MESS_MAX MESS_MAX-2
#define FILE_MESS_MAX MESS_MAX-7
//#define MAX_INT_NUM 360
unsigned char  sequence;
extern char query_result[4];
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
	COMMAND,
};

typedef struct frame {
	unsigned char version = FRAME_VERSION;//已经填入了值
	unsigned char pro;
	int MESS_LEN;
	char MESS[MESS_MAX];
}fram;
/*
struct integer {
	int N;
	char len_seq[MAX_INT_NUM/8];
	char int_seq[MAX_INT_NUM];
};
*/
struct chap {
	unsigned char type;
	unsigned char  sequence;
	char mess[CHAP_MESS_MAX];
};

struct file {
	char type;//DATA or ACK
	char sequence;//0 or 1
	char filesequence;//from 0 to ... 
	int len;//len of data
	char data[FILE_MESS_MAX];
};

struct clos
{
	char  sequence;
};
struct err
{
	char type;
};

//函数:fram* createframe()
//功能:生成给定类型、消息内容的待发送帧
fram* createframe(enum protocol pro,int  mess_len,char* mess) {
	fram fra;
	fra.pro = pro;
	fra.MESS_LEN = mess_len;
	for (int i = 0; i < mess_len; i++)
		fra.MESS[i] = mess[i];
	//fra.MESS[mess_len] = '\0';
	return &fra;
}

char* creat_file_message(int type, int sequence, int filesequence, int data_len, char* data) {
	struct file* f = (struct file*)malloc(sizeof(struct file));
	f->type = type;
	f->sequence = sequence;
	f->filesequence = 0;//无效
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

int get_frame_len(fram* fra) {
	return ((char*)(fra->MESS) - (char*)fra) + fra->MESS_LEN;
}

int set_rand(int rand_nu, int rand_opt, int opt);
//该函数返回frame中mess字段,prob为整数长度为四字节的概率*100
int creat_chap_query_mess(struct chap* chap1,int prob) {
	chap1->type = 0x00;
	chap1->sequence = set_rand(0,256,1);
	sequence = chap1->sequence;
	int N = set_rand(0,256,8);
	char *len_seq = (char*)malloc(sizeof(char)*(N/8)+1);
	int num = N;
	for (int i = 0; i < N / 8; i++) {
		int seq = 0;
		for (int j = 0; j < 8; j++) {
			seq *= 2;
			int flag = set_rand(0, 100, 1) >= prob ? 0 : 1;
			seq += flag;
			num += flag;
		}
		len_seq[i] = seq;
	}

	chap1->mess[0] = N ;

	//strcat(chap1->mess, len_seq);
	for (int i = 0; i < N / 8; i++) {
		chap1->mess[i + 1] = len_seq[i];
	}
	for (int i = 0; i < num * 2; i++)
		chap1->mess[1 + N / 8 + i] = set_rand(0,256,1);
	chap1->mess[1 + N / 8 + num * 2] = '\0';
	//chap1.mess
	printf("N:%d num:%d\n", N, num);
	return 1 + N / 8 + num * 2 + 1;
}
//rand_nu is useless,% rand_opt and = k*opt
int set_rand(int rand_nu,int rand_opt,int opt) {
	rand_nu = rand() % rand_opt;
	rand_nu += rand_nu >= opt ? 0 : opt;
	//printf("%d\n", rand_nu - rand_nu % opt);
	return rand_nu-rand_nu%opt;
}

char* creat_chap_result_mess(int result) {
	struct chap *chap2 = (struct chap*)malloc(sizeof(struct chap));
	chap2->type = 0x02;
	chap2->sequence = sequence;
	chap2->mess[0] = result;
	return (char*)chap2;
}

void get_query_result(unsigned char* chap_mess, int key,char* query_result) {
	int N = chap_mess[0];
	int num = 0;
	unsigned char* seq_len = chap_mess+1;
	unsigned char* seq = chap_mess + 1 + N / 8;
	for (int i = 0; i < N/8; i++) {
		for (int j = 0; j < 8; j++) {
			int k = (seq_len[i] >> (7 -j)) & 0x01;
			if (k) { num += seq[0] << 24 + seq[1] << 16 + seq[2] << 8 + seq[3]; seq = seq + 4; }
			else { num += seq[0] << 8 + seq[1]; seq = seq + 2; }
			//这里没有考虑int溢出问题，不知道结果会不会自动删除溢出
		}
	}
	num = num ^ key;
	for (int i = 0; i < 4; i++)
		query_result[i] = num >> i * 8;
	//return (char*)(&num);
}
