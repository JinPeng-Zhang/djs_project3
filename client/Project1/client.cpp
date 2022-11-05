#include<stdio.h>
#include"frame.h"
#include<winsock.h>
#pragma comment(lib, "WS2_32")
int main() {
	//windowsÆô¶¯socket·þÎñ
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
	char* message = (char*)malloc(sizeof(char) * 20);
	recv(s, message, 20, 0);
	fram* fra = (fram*)message;
	printf("%d", fra->pro);
	return 0;
}