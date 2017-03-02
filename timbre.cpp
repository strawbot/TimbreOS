// Timbre++  Robert Chapman III  Feb 23, 2017

class Timestamp {
	Cell time;
	Cell getTime();
};

class Timeout : public Timestamp {
	void start();
	void stop();
	void set(Cell time);
	void repeat();
};

class Queue {
	void push(Cell n);
	Cell pull();
	void stuff(Cell n);
	Cell pop();
	Cell left();
	Cell full();
	Cell p();
	Cell q();
	Queue(size)
};

class ByteQ : public Queue {
};

// variable state is run when active
// machine states are set to state
// states can be active or not
// each machine instance is linked back to the Machine class so they can be executed
// There is no notion of activate each time, machines just run
// activate and deactivate are argument less and apply to instance
// Machine class
class Machine : public Queue {
	void activate();
	void deactivate();
	void runMachines();// runMachines is static class member which will run all children's states if active
};

class StateMachine : public Machine {
	vector state;
	void wait(bool condition);
	void goto(vector next) { state = next; }
};

class TimeMachine : public Machine, Timeout {
	virtual void wait(bool condition, Cell time);
};

/* Usage:
	Machine m1;
	m1.machine() { ... }
	
	class Blink : public TimeMachine {
		Cell index, limit;
		Cell **times;
		void machine () { if (index & 1) ledOn() else ledOff(); wait(false, times[index++]); if (index == limit) index = 0; }
		Blink ( Cell intervals[] ) { times = intervals; limit = elementsof(intervals); index = 0; }
	};
	Blink blink([500, 500]);
	Blink blinkBlink([700, 100, 100, 100]);
	Blink blinkBlonk([100, 100, 100, 700]);
*/