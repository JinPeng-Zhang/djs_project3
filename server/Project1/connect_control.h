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
	EVENT_RECV0,
	EVENT_RECV1,
};
enum Event event = EVENT_Nothing;
