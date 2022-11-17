#include<stdio.h>
#include<winsock.h>
#pragma comment(lib, "WS2_32")
#include"frame.h"
#include"connect_control.h"
#include<iostream>
#include<windows.h>
using namespace std;

WSADATA wsaData;
SOCKET s;
FILE* Fid_read, *Fid_write;
int Retval;
char* frame_recv = (char*)malloc(sizeof(char) * 1600);
fram* fr;
struct chap* chap1;
struct file* file1;
struct resend {
	char data_resend[FILE_MESS_MAX];
	int len;
};
struct resend* resend1= (struct resend*)malloc(sizeof(resend));
int l=0;//记录发送帧中数据长度

char recvfilesequence = 0;//用于判断目前接收文件和上一接受文件关系，保存上一接收文件标识

void Init();
void End();
int chap();
void S0_fsm();
void S1_fsm(); 
void End_fsm();
int SendFile(int sequence);
void RsendFile(int sequence);
void ready_fsm();
void R0_fsm();
void R1_fsm();
void SendACK(int towhichframe);
void ShowMENU();
void SendCommand(int whichorder);
void ShowrecvFileDir();

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
	printf("connect to server\n");

	fd_set read_list;
	timeval timeout;
	unsigned long arg;
	arg = 1;
	ioctlsocket(s, FIONBIO, &arg);
	/****以上为初始化定义****/

	ShowMENU();
	int n;
	do {
		scanf_s("%d", &n, 4);
	} while (n > 3 || n < 1);
	//发送帧0
	switch (n) {
	case 1:
	{
		l = SendFile(0);
		if (l < FILE_MESS_MAX) {
			status = SENDEND;
		}
		else {
			status = SEND0;
		}
		break;
	}
	case 2:
	{
		//获取文件名
		
		//发送文件请求
		SendCommand(download_requst);
		status = READY;
		break;
	}
	case 3:
	{
		SendCommand(checkfiledir);
		status = READY;
		break;
	}
	}
	/*l = SendFile(0);
	if (l < FILE_MESS_MAX) {
		status = SENDEND;
	}
	else {
		status = SEND0;
	}*/

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
			Retval = recv(s, frame_recv, 1600, 0);
			printf("I recv %d byte,", Retval);
			if (Retval <= 0) {
				Retval = WSAGetLastError();
				if (Retval == WSAEWOULDBLOCK)
					continue;
				printf("Connection was closed");
				End();//连接断开，释放套接字，服务器等待新的连接
				return 0;
			}//检测到断开连接
			fr = (fram*)frame_recv;
			if (fr->pro == FILE_OPT) {
				file1 = (struct file*)fr->MESS;
				if (file1->type == DATA) {
					if (file1->sequence == 0) event = EVENT_RECV0;
					else if (file1->sequence == 1) event = EVENT_RECV1;
					else if (file1->sequence == 2) event = EVENT_RECVFILEDIR;
				}
				else if (file1->type == ACK) {
					if (file1->sequence == 0) event = EVENT_RECVACK0;
					else if (file1->sequence == 1) event = EVENT_RECVACK1;
				}
			}
			else if (fr->pro == CHAP) {
				//认证状态
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
		case READY:
			ready_fsm();
			break;
		case RECV0:
			R0_fsm();
			break;
		case RECV1:
			R1_fsm();
			break;
		default:
			End();
		}
	}


	return 0;
}

int chap() {
	//接收质询报文
	char* message = (char*)malloc(sizeof(char) * 1500);
	recv(s, message, 1500, 0);
	//拆封
	fram* fra = (fram*)message;
	struct chap* chap1 = (struct chap*)fra->MESS;
	//参数设置
	sequence = chap1->sequence;
	//异或运算
	char query_result[4];
	get_query_result((unsigned char*)chap1->mess, key, query_result);
	//构建ack报文
	char* ack_mess = creat_chap_ack_mess(query_result);
	fram* fra_ack = createframe(CHAP, strlen(ack_mess), ack_mess);
	//发送ack报文
	send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	//接收报文观察质询是否成功
	recv(s, message, 1500, 0);
	fra = (fram*)message;
	chap1 = (struct chap*)fra->MESS;
	int chap_success = chap1->mess[0];
	if (chap_success == 1)printf("chap success");
	else printf("chap failed");
	return chap_success;
}

//函数：Init()
//功能: 初始化，套接字初始化，开启监听，文件函数初始化
void Init() {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!");
	
	fopen_s(&Fid_read,"E:\\2022full_netprogram_djs\\djs_project3\\client\\client_snd.txt", "rb");
	if (Fid_read == NULL) {
		printf("can`t open file\n");
		return;
	}
	fopen_s(&Fid_write, "E:\\2022full_netprogram_djs\\djs_project3\\client\\client_rx.txt", "wb");
	if (Fid_write == NULL) {
		printf("can`t open file\n");
		return;
	}

}

//函数：End()
//功能: 关闭套接字，关闭文件函数
void End() {
	closesocket(s);
	fclose(Fid_read);
	WSACleanup();
}

void End_fsm()
{
	switch (event) {
	case EVENT_RECVACK0: {
		//接收到最后一帧 帧0的ACK，发送完毕
		printf("Recv ACK of Thelastframe.End\n");
		status = END;
		closesocket(s);
		break;
	}
	case EVENT_RECVACK1: {
		// 接收到最后一帧 帧1的ACK，发送完毕
		printf("Recv ACK of Thelastframe.End\n");
		status = END;
		closesocket(s);
		break;
	}
	case EVENT_TIMEOUT:
		//超时，重发上一帧
		RsendFile(10);
		status = SENDEND;
		break;
	}
}

void S0_fsm()
{
	switch (event) {
	case EVENT_RECVACK0: {
		//接收到帧0的ACK，发送下一帧 帧1
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
		//超时，重发帧0
		RsendFile(0);
		status = SEND0;
		break;
	}
}

void S1_fsm()
{
	switch (event) {
	case EVENT_RECVACK1: {
		//接收到帧1的ACK，发送下一帧 帧0
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
		//超时，重发帧1
		RsendFile(1);
		status = SEND1;
		break;
	}
}

//函数:int SendFile()
//功能：按照FILE_MESS_MAX依次读取文件，发送帧x(sequence)
int SendFile(int sequence) {
	char data[FILE_MESS_MAX];
	int len = fread(data, sizeof(char), sizeof(data), Fid_read);//实际读取到了len大小,读满len=FILE_MESS_MAX
	for (int i = 0; i < FILE_MESS_MAX; i++) {
		resend1->data_resend[i] = data[i];
	}
	resend1->len = len;//
	char* file_message = creat_file_message(DATA, sequence, 0, len, data);//filesequence待定
	fram* fra_filedata = createframe(FILE_OPT, 7 + len, file_message);
	send(s, (char*)fra_filedata, get_frame_len(fra_filedata), 0);
	printf("Send frame %d,send %d byte.\n\n", sequence, get_frame_len(fra_filedata));
	return len;
}

//函数:void RsendFile()
//功能：resend1保存上一次发送的数据和数据长度，重新发送帧x(sequence)
void RsendFile(int sequence) {
	int len = resend1->len;
	char* file_message = creat_file_message(DATA, sequence, 0, len, resend1->data_resend);//参数filesequence待定
	fram* fra_filedata = createframe(FILE_OPT, 7 + len, file_message);
	send(s, (char*)fra_filedata, get_frame_len(fra_filedata), 0);
	if (sequence > 2)
		printf("Time out!Rsend Thelast frame,send %d byte.\n\n", get_frame_len(fra_filedata));
	else
		printf("Time out!Rsend frame %d,send %d byte.\n\n", sequence, get_frame_len(fra_filedata));
	return;
}

/*printf("%d\n", frame_send->pro);
		printf("%d\n", frame_send->MESS_LEN);
		file* ff = (struct file*)frame_send->MESS;
		printf("%d\n", ff->type);
		printf("%d\n", ff->len);
		for(int i=0;i<ff->len;i++)
		printf("%d\n", ff->data[i]);
		*/

/***以下是接收处理函数***/
void ready_fsm()
{
	switch (event) {
	case EVENT_RECV0: {
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧0 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//返回ACK
		SendACK(0);
		status = RECV0;
		break;
	}
	case EVENT_RECV1: {
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧1 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//返回ACK
		SendACK(1);
		status = RECV1;
		break;
	}
	case EVENT_RECVFILEDIR: {
		ShowrecvFileDir();
		SendACK(2);
		status = READY;
		break;
	}
	default://也可能是其他情况，待补充

		break;
	}
}

void R0_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 不写入文件
		//返回ACK
		SendACK(0);
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧1 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//返回ACK
		SendACK(1);
		status = RECV1;
		break;
	}
}

void R1_fsm()
{
	switch (event) {
	case EVENT_RECV0:
		//接收到帧0 写入文件
		if (file1->filesequence == recvfilesequence) {
			// 接收到帧1 写入文件
			fwrite(file1->data, sizeof(char), file1->len, Fid_write);
		}
		//返回ACK
		SendACK(0);
		status = RECV0;
		break;
	case EVENT_RECV1:
		//接收到帧1 写入文件
		//返回ACK
		SendACK(1);
		status = RECV1;
		break;
	}
}

void SendACK(int towhichframe) {
	char* ack_message = creat_file_message(ACK, towhichframe, 0, 0, NULL);
	fram* fra_ack = createframe(FILE_OPT, 7, ack_message);
	send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	printf("Recv frame %d,Send ACK!\n", towhichframe);
	return;
}

void SendCommand(int whichorder) {
	//whichorder=0 download_requst/whichorder=1 checkfiledir
	if (whichorder == download_requst) {
		//char* file_name;
		//int filename_len;
		char* command_message = creat_file_message(COMMAND, whichorder, 0, 0, NULL);
		fram* fra_ack = createframe(FILE_OPT, 7, command_message);
		send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
		printf("SendCommand download_requst\n");
	}
	else if (whichorder == checkfiledir) {
		char* command_message = creat_file_message(COMMAND, whichorder, 0, 0, NULL);
		fram* fra_ack = createframe(FILE_OPT, 7, command_message);
		send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
		printf("SendCommand checkfiledir\n");
	}
	return;
}
/****以下是命令处理函数*****/
void ShowMENU() {
	printf("\nPlease choose:\n");
	printf("1.Upload\n");
	printf("2.Download\n");
	printf("3.Show File Directory\n");
	return;
}

void ShowrecvFileDir() {
	printf("\nFileDir Recved:\n");
	char Dirdata[1500];
	for (int i = 0; i < file1->len; i++) {
		Dirdata[i] = file1->data[i];
	}
	Dirdata[file1->len] = 0;
	cout << Dirdata << endl;
	return;
}