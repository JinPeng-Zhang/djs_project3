#pragma once
enum Status {
	Nothing,
	UNCHAPED,
	READY,//client׼�������ļ�
	RECV0,
	RECV1,
	SEND0,//�ѷ���֡0,����ACK0
	SEND1,//�ѷ���֡1,����ACK1
	SENDEND,//�Ѿ��������һ֡
	END,//���һ֡�ѱ����գ��Ͽ�����
	clos,
};
enum Status status = UNCHAPED;

enum Event {
	EVENT_Nothing,
	EVENT_RECV0,//�Ѿ����յ�֡0���ڴ�֡1
	EVENT_RECV1,
	EVENT_RECVACK0,
	EVENT_RECVACK1,
	EVENT_SENDCHAPANS,
	EVENT_RECVCHAPANS,
	EVENT_TIMEOUT,
};
enum Event event = EVENT_Nothing;

#define download_requst 0
#define checkfiledir 1