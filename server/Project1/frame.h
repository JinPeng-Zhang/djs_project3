#pragma once
//frame.h需要包含帧结构体定义和处理函数
#define FRAME_VERSION 0X01
#include<stdlib.h>
#include<time.h>
#include<math.h>
#define MESS_MAX 1494
#define CHAP_MESS_MAX MESS_MAX-2
//#define MAX_INT_NUM 360
char  sequence;
enum protocol
{
	CHAP=0X01,
	FILE_OPT,
	CLOSE,
	ERR,
};
typedef struct frame {
	enum protocol pro;
	unsigned char version = FRAME_VERSION;
	unsigned char MESS_LEN;
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

struct clos
{
	char  sequence;
};
struct err
{
	char type;
};
fram* initframe(enum protocol pro,unsigned char mess_len,char* mess) {
	fram fra;
	fra.pro = pro;
	fra.MESS_LEN = mess_len;
	for (int i = 0; i < mess_len; i++)
		fra.MESS[i] = mess[i];
	fra.MESS[mess_len] = '\0';
	return &fra;
}
int get_frame_len(fram* fra) {
	return ((char*)(fra->MESS) - (char*)fra) + fra->MESS_LEN+1;
}
int set_rand(int rand_nu, int rand_opt, int opt);
//该函数返回frame中mess字段,prob为整数长度为四字节的概率*100
char* creat_chap_query_mess(int prob) {
	struct chap chap1;
	chap1.type = 0x00;
	chap1.sequence = set_rand(0,256,1);
	sequence = chap1.sequence;
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
	len_seq[N / 8] = '\0';

	chap1.mess[0] = N ;
	chap1.mess[1] = '\0';

	strcat(chap1.mess, len_seq);

	for (int i = 0; i < num * 2; i++)
		chap1.mess[1 + N / 8 + i] = set_rand(0,256,1);
	chap1.mess[1 + N / 8 + num * 2] = '\0';
	//chap1.mess
	return (char*)(&chap1);
}
//rand_nu is useless,% rand_opt and = k*opt
int set_rand(int rand_nu,int rand_opt,int opt) {
	rand_nu = rand() % rand_opt;
	rand_nu += rand_nu >= opt ? 0 : opt;
	//printf("%d\n", rand_nu - rand_nu % opt);
	return rand_nu-rand_nu%opt;
}

char* creat_chap_result_mess(int success) {
	struct chap *chap1;
	chap1->type = 0x02;
	chap1->sequence = sequence;
	chap1->mess[0] = success;
	return (char*)chap1;
}
char* get_query_result(char* chap_mess, char* key) {
	int N = chap_mess[0];
	int num = 0;
	for (int i = 0; i < N/8; i++) {

	}
	return NULL;
}