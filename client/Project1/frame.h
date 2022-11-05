#pragma once
//frame.h需要包含帧结构体定义和处理函数
#define FRAME_VERSION 0x01
#include<stdlib.h>
#define MESS_MAX 300
enum protocol
{
	CHAP = 0x02,
	FILE_OPT,
	CLOSE,
	ERR,
};
typedef struct frame {
	unsigned char version = FRAME_VERSION;
	enum protocol pro;
	unsigned char MESS_LEN;
	char MESS[MESS_MAX];
}fram;
struct chap {
	char type;
	char  sequence;
	void* mess;
};
struct file_opt
{
	char type;
	char filename[20];
	void* mess;
};
struct clos
{
	char  sequence;
};
struct err
{
	char type;
};
fram* initframe(enum protocol pro, unsigned char mess_len, char* mess) {
	fram fra;
	fra.pro = pro;
	fra.MESS_LEN = mess_len;
	for (int i = 0; i < mess_len; i++)
		fra.MESS[i] = mess[i];
	/*switch (fra.pro)
		{
		case CHAP:
			fra.MESS = ;
			break;
		case FILE_OPT:
			fra.MESS = (struct file_opt*)malloc(sizeof(struct file_opt));
			break;
		case CLOSE:
			fra.MESS = (struct clos*)malloc(sizeof(struct clos));
			break;
		case ERR:
			fra.MESS = (struct err*)malloc(sizeof(struct err));
			break;
		default:
			return NULL;
		}*/
	return &fra;
}
