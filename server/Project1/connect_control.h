#pragma once
enum Status {
	Nothing,
	UNCHAPED,
	READY,
	RECV0,
	RECV1,
	SEND0,
	SEND1,
	clos,
};
enum Status status = UNCHAPED;

enum Event {
	EVENT_Nothing,
	EVENT_RECV0,//�Ѿ����յ�֡0���ڴ�֡1
	EVENT_RECV1,
	EVENT_RECVACK0,
	EVENT_RECVACK1,
	EVENT_RECVCHAPREQ,
	EVENT_RECVCHAPANS,
	EVENT_TIMEOUT,
};
enum Event event = EVENT_Nothing;
