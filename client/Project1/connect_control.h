#pragma once
#pragma once
enum Status {
	Nothing,
	UNCHAPED,
	READY,
	RECV0,
	RECV1,
	SEND0,//已发送帧0,待收ACK0
	SEND1,//已发送帧1,待收ACK1
	SENDEND,//已经发送最后一帧
	END,//最后一帧已被接收，断开连接
	clos,
};
enum Status status = UNCHAPED;

enum Event {
	EVENT_Nothing,
	EVENT_RECV0,
	EVENT_RECV1,
	EVENT_RECVACK0,
	EVENT_RECVACK1,
	EVENT_SENDCHAPANS,
	EVENT_RECVCHAPANS,
	EVENT_TIMEOUT,
};
enum Event event = EVENT_Nothing;
