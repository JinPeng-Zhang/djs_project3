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
	//windows启动socket服务
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
	//接收质询报文
	char* message = (char*)malloc(sizeof(char) * 1500);
	recv(s, message, 1500, 0);
	printf("收到CHAP质询\n");
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
	fram* fra_ack = initframe(CHAP, strlen(ack_mess), ack_mess);
	//发送ack报文
	send(s, (char*)fra_ack, get_frame_len(fra_ack), 0);
	printf("返回ACK回应\n");
	//接收报文观察质询是否成功
	recv(s, message, 1500, 0);
	fra = (fram*)message;
	chap1 = (struct chap*)fra->MESS;
	int chap_success = chap1->mess[0];
	if (chap_success == 1)printf("chap success");
	else printf("chap failed");
	//状态机流程变量预定义
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
				//从键盘获取命令
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
				//这里需要一个cmd判断函数，即判断输入的命令是否合法...
				if (opt_check(opt, &file))stat = deal_cmd;
				else { printf("输入命令有误，请重新输入\n"); }
				break;
			case deal_cmd:
				//根据cmd命令产生指令类型报文
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
				//接收文件内容
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

/*差错控制：①对于报文丢失的考虑，
一个是发送文件内容丢失，设置ack超时重传；
再者是ack丢失，发送端重传接收到重复报文,需要接收方增加个报文重复检测机制，重复报文不进行读写
，但是都需要返回ack
对于读取指令和接收报文阻塞问题的考虑：
阻塞：当一个指令完成时，阻塞接受指令，但当上一个指令操作存在ack丢包问题时，对方重发报文
因此需要在接收到指令的2*timeout需要检测出上一个指令最后一个报文是否收到ack,
或者在最后一个ack发送后检测3*timeout的时间，在进入命令接受阶段
②对于连续三个报文发送后都没有收到ack，我们认定为链路丢包率太高，主动断开文件发送过程
接收方：可能是三个报文都丢失了，也可能是丢了几个ack,
需要一个判定过程，文件分片了，但是一定时间内未收到后续报文，也需要一个超时机制
即3*timeout
*/


/*
总结：
帧丢失处于指令以及文件发送ack阶段(除CHAP，这段可以细化状态机进而也增加报文丢失过程)
所有报文都设置重传，但根据作用和时间分为三种
发送报文含文件内容或指令的，超时时间为timeout，超市了重传，当需要重传第三次时主动断开连接
ack报文设置超时时间为3.5*timeout,超时断开连接（这个分为两种断开连接，一个是网络连续丢包断开发送，另一种是完成指令，最后一个ack）。

*/