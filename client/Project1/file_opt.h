#pragma once
#include"frame.h"
#include<string.h>
#include<winsock.h>
#include<time.h>
#include<math.h>
#pragma comment(lib, "WS2_32")
#define FIEENAME_MAXLEN 100
#define FILE_MESS_MAXLEN MESS_MAX-FIEENAME_MAXLEN-2
FILE* ffile;
int packet_seq = 0;
int resend_time = 0;
int err_prob = 20;
int  checkfile(char* filename);
void writefile(struct fil* fi, int len);
void printfile(struct fil* fi);
void send_(SOCKET s, struct fil fi, int len);
void sendack(SOCKET s, int packet_seq);
int error_prob(int prob);
struct fil {
	unsigned char type;
	unsigned char seq;
	char filename[FIEENAME_MAXLEN];
	char message[FILE_MESS_MAXLEN];
};

enum fileopt {
	SHOW = 0x03,
	DOWNLOAD,
	UPLOAD,
	ACK,
};
struct file_opt {
	int num;
	char opt[2][100];
};
enum status {
	wait_cmd,
	deal_cmd,
	send_frame,//两种，download 和 ls
	send_file,
	recv_file,
	wait_ack,
	finish_cmd,
	clo,
	//这里有个漏洞，如果接受报文后，客户机ack丢失，
};
struct fil ff;
int opt_check(struct file_opt opt, struct fil* file) {
	if (opt.num >2 || opt.num <1) return 0;
	file->seq = 0;
	packet_seq = 0;
	if (opt.num == 1 && !strncmp(opt.opt[0], "show", 4)) {
		file->type = SHOW;
		file->filename[0] = '\0';
	}
	if (opt.num == 2) {
		if (!strncmp(opt.opt[0], "download", 8)) { file->type = DOWNLOAD; strcpy(file->filename, opt.opt[1]); file->filename[strlen(file->filename) - 1] = '\0'; ffile = fopen(file->filename, "wb"); }
		else if (!strncmp(opt.opt[0], "upload", 6)) { file->type = UPLOAD; strcpy(file->filename, opt.opt[1]); file->filename[strlen(file->filename) - 1] = '\0'; ffile = fopen(file->filename, "rb");}
		else if (!strncmp(opt.opt[0], "show", 4)) { file->type = SHOW; strcpy(file->filename, opt.opt[1]); }
		else return 0;
	return 1;
	}
}
//处理收到的报文，先比较函数输入packet_seq和全局变量packet_seq大小关系判断是否为重传的
//在将报文处理（文件 or 文件夹），在发送ack，返回报文中的标志位
int deal_recv_file(SOCKET s,char* file_mess, int* packet_seq) {
	fram* fra = (fram*)file_mess;
	struct fil* fi = (struct fil*)fra->MESS;
	if (fi->seq > *packet_seq) {
		*packet_seq = fi->seq;
		int filecheak = checkfile(fi->filename);
		if (filecheak) writefile(fi, fra->MESS_LEN-102);
		else printfile(fi);
		sendack(s, *packet_seq);
	}
	else sendack(s,*packet_seq);
	if (fi->seq == 255) { 
		int filecheak = checkfile(fi->filename);
		if (filecheak) fclose(ffile);
		return 1; }
	return 0;
}
//发送文件信息，设置分片，返回分片标志位
int send_file_mess(SOCKET s,struct fil file, int seq) {
	if (resend_time != 0) {
		send_(s, ff, strlen(ff.message));
		printf("i resend %d\n", ff.seq);
		return ff.seq == 255 ? 1 : 0;
	}
	if (feof(ffile))
	{
		file.seq = 255;
		packet_seq = 255;
		send_(s, file,0);
		return 1;}
	else {
		int size = fread(file.message,1, FILE_MESS_MAXLEN-3,ffile);
		file.seq = seq;
		printf("size:%d\n",size);
		send_(s,file,size);
	}
	return 0;
}
//返回1代表文件，0代表文件夹
int  checkfile(char* filename) {
	if (strchr(filename, '.'))return 1;
	return 0;
}
void writefile(struct fil* fi,int len) {
	fwrite(fi->message,len ,1,ffile);
}
void sendack(SOCKET s, int seq) {
	struct fil fi;
	fi.type = ACK;
	fi.seq = seq;
	fram fra;
	fram* frap = initframe(FILE_OPT, 2, (char*)(&fi));
	fra.pro = frap->pro;
	fra.version = frap->version;
	fra.MESS_LEN = frap->MESS_LEN;
	strcpy(fra.MESS, frap->MESS);
	//printf("i send ack :%d\n", seq);
	if (error_prob(err_prob)) send(s, (char*)(&fra), get_frame_len(&fra), 0);
	else printf("丢了ACK\n");
	//if (a == -1)printf("send error %d\n", WSAGetLastError());
}
void ack_ckeck(char* message, int seq) {
	fram* fra = (fram*)message;
	struct fil* fi = (struct fil*)fra->MESS;
	if (fi->seq == seq && fi->type == ACK) {
		printf("get ack\n");
	}
	else { printf("it is not ack\n"); printf("%d %d\n", fi->seq,seq); }
}
void printfile(struct fil* fi) {
	//puts(fi.message);
	printf("%s\n", fi->message);
}
void send_(SOCKET s, struct fil fi,int len) {
	fram* fra = initframe(FILE_OPT, 102+len, (char*)(&fi));

	ff.type = fi.type;
	ff.seq = fi.seq;
	strcpy(ff.filename, fi.filename);
	strncpy(ff.message, fi.message,len);
	if(error_prob(err_prob))send(s, (char*)(fra), get_frame_len(fra), 0);
	else printf("丢了PACKET\n");
	printf("i send \"messagelen:%d\":%d\n", len, fi.seq);
}
int error_prob(int prob) {
	if (abs(rand()) % 100 < prob) {
		return 0;
	}
	return 1;
}
