#pragma once
#ifndef Evt_info
#define Evt_info
#include <cstdint>

template <typename Type>
class property {
private:
	Type variable = Type();
public:
	property() = default;
	~property() = default;
	Type Get() { return variable; };
	Type Set(Type value) { return variable = value; };
	Type operator()() { return variable; };
	Type operator()(Type value) { return variable = value; };
	Type operator=(Type value) { return variable = value; };
	operator Type() { return variable; };
};

class Event_Information {
public:
	Event_Information() = default;
	~Event_Information() = default;
	property<int64_t> 
		EventId,
		RunId;
};

#endif // !Evt_info