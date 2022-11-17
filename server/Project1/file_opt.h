#pragma once
#define FIEENAME_MAXLEN 100
#define FILE_MESS_MAXLEN MESS_MAX-FIEENAME_MAXLEN-2
#include<direct.h>
#include<io.h>
#include<winsock.h>
extern int resend_time;
#pragma comment(lib, "WS2_32")
FILE* ffile;
int packet_seq = 0;
char chRE[] = "*.*";
char path[] = "D:\\djs_project3\\djs_project3\\file\\";
int Ret;
long long hnd;
int err_prob = 20;
struct _finddata_t data;
int  checkfile(char* filename);
void writefile(struct fil* fi, int len);
void printfile(struct fil fi);
void send_(SOCKET s, struct fil fi, int len);
void sendack(SOCKET s, int packet_seq);
int error_prob(int prob);
void showlist();
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
enum status {
	wait_cmd,
	send_frame,//两种，download 和 ls
	send_file,
	recv_file,
	wait_ack,
	finish_cmd,
	clo,
	//这里有个漏洞，如果接受报文后，客户机ack丢失，
};
struct fil ff;
int deal_recv_file(SOCKET s,char* file_mess, int *packet_seq) {
	fram* fra = (fram*)file_mess;
	struct fil* fi = (struct fil*)fra->MESS;
	if (fi->seq > *packet_seq) {
		*packet_seq = fi->seq;
		int filecheak = checkfile(fi->filename);
		if (filecheak) { writefile(fi, fra->MESS_LEN - 102); printf("writefile %d\n", fra->MESS_LEN - 102); }
		sendack(s,*packet_seq);
	}
	else sendack(s,*packet_seq);
	if (fi->seq == 255) {
		printf("close file");
		fclose(ffile);
		return 1;
	}
	return 0;
}
int send_file_mess(SOCKET s,struct fil file, int seq) {
	if (resend_time != 0) {
		send_(s, ff, strlen(ff.message)+1);
		printf("i resend %d\n",ff.seq);
		return ff.seq == 255 ? 1:0;
	}
	//判断出需要发送的是文件还是文件夹
	if (file.type == DOWNLOAD) {
		if (feof(ffile))
		{
			file.seq = 255;
			packet_seq = 255;
			send_(s, file, 0); return 1;
		}
		else {
			int size = fread(file.message, 1, FILE_MESS_MAXLEN - 3, ffile);
			file.seq = seq;
			printf("size:%d\n", size);
			send_(s, file, size);
		}
	}
	else if(file.type == SHOW) {
		file.message[0] = '\0';
		if (Ret == -1) {
			file.seq = 255;
			packet_seq = 255;
			send_(s, file, 0);
			_findclose(hnd);
			return 1;
		}
		else {
			file.seq = seq;
			if (data.attrib == _A_SUBDIR) {
				strcpy(file.message, data.name);
				file.message[strlen(data.name)] = '*';
				file.message[strlen(data.name) + 1] = '\0';
				send_(s, file, strlen(data.name)+2);
				//printf("datalen:%d\n")
			}
			else{
				strcpy(file.message, data.name);
				send_(s, file, strlen(data.name)+1);
			}
			Ret = _findnext(hnd, &data);
		}
	}
	return 0;
}
enum status deal_cmd(SOCKET s,char* message, struct fil* file) {
	fram* fra = (fram*)message;
	struct fil* files = (struct fil*)fra->MESS;
	//
	file->type = files->type;
	file->seq = files->seq;
	printf("cmdseq:%d\n", file->seq);
	strncpy(file->filename, files->filename,100);
	//
	packet_seq = file->seq+1;
	if (file->type == SHOW) {
		int a;
		if(strlen(file->filename)==0)
		a = !_chdir("D:\\djs_project3\\djs_project3\\file");
		else { file->filename[strlen(file->filename)-1] = '\0';
		a = !_chdir(strcat(path, file->filename)); }
		printf("%s change %s  %d\n", path,a?"success":"failed",strlen(file->filename));
		//showlist();
		hnd = _findfirst("*.*", &data);
		if (hnd < 0) { printf("文件夹读取错误\n"); return clo; }
		Ret = 1;
		printf("i get SHOW cmd\n");
		printf("file type %d\n", file->type);
		sendack(s, file->seq);
		return send_file; 
	}
	if (file->type == DOWNLOAD) { 
		file->filename[strlen(file->filename)] = '\0'; 
		ffile = fopen(file->filename,"rb"); 
		if (ffile == NULL)printf("打开%s失败\n", file->filename);
		sendack(s, file->seq); return send_file; }
	if (file->type == UPLOAD) { 
		file->filename[strlen(file->filename)] = '\0'; 
		ffile = fopen(file->filename,"wb"); sendack(s, file->seq); return recv_file; }
	//return clo;
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
	//printf("i send ack socket:%s\n", frap);
	if (error_prob(err_prob))  send(s, (char*)(&fra), get_frame_len(&fra), 0);
	else printf("丢了ACK\n");
	//if (a == -1)printf("send error %d\n", WSAGetLastError());
}
void ack_ckeck(char* message, int packet_seq) {
	printf("check ack...");
	fram* fra = (fram*)message;
	struct fil* fi = (struct fil*)fra->MESS;
	if (fi->seq == packet_seq && fi->type == ACK) {
		printf("get ack %d\n", fi->seq);
	}
	else { printf("it is not ack with eq = "); printf("%d\n", fi->seq); }
}
int  checkfile(char* filename) {
	if (strchr(filename, '.'))return 1;
	return 0;
}
void writefile(struct fil* fi, int len) {
	fwrite(fi->message, len, 1, ffile);
}
void send_(SOCKET s, struct fil fi, int len) {
	fram* fra = initframe(FILE_OPT, len + 102, (char*)(&fi));
	ff.type = fi.type;
	ff.seq = fi.seq;
	strcpy(ff.filename, fi.filename);
	strncpy(ff.message, fi.message,len);
	if (error_prob(err_prob)) send(s, (char*)fra, get_frame_len(fra), 0);
	else printf("丢了PACKET\n");
	printf("i send \"messagelen:%d\":%d", len,fi.seq);
}
int error_prob(int prob) {
	if (abs(rand()) % 100 < prob) {
		return 0;
	}
	return 1;
}
//此处为验证代码，遇到了一运行到_findnext程序闪退，原因_findnext第一个参数对于win7使用long,win10使用long long
/*
void showlist() {
	char chRE1[] = "*";
	char path1[] = "D:\\djs_project3\\djs_project3\\server\\Project1";
	int Ret1 = 1;
	long long hnd1;
	struct _finddata_t data1;
	_chdir(path1);
	hnd1 = _findfirst(chRE1, &data1);
	if (hnd1 < 0) { printf("文件夹读取错误\n"); }
	else printf("i get SHOW cmd\n");
	while (Ret1 != -1)
	{
		printf("%s\n", data1.name);
		Ret1 = _findnext(hnd1, &data1);
	}
	system("pause");
}*/