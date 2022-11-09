#include<stdio.h>
#include"frame.h"
#include<winsock.h>
#pragma comment(lib, "WS2_32")
int main() {
	//windows启动socket服务
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup() error!\n");

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	SOCKET new_s;
	struct sockaddr_in saddr, new_addr;


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
	//接收报文观察质询是否成功
	recv(s, message, 1500, 0);
	fra = (fram*)message;
	chap1 = (struct chap*)fra->MESS;
	int chap_success = chap1->mess[0];
	if (chap_success == 1)printf("chap success");
	else printf("chap failed");
	system("pause");
	while(chap_success) {

	}
	return 0;
}