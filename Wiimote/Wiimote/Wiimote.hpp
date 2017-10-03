#pragma once

#include <mutex>

class Wiimote {
private:
	const unsigned short VID = 0x057e;
	const unsigned short PID = 0x0306;

	void* wiihandle;
	unsigned short input_length, output_length;

	enum WriteMethod {
		File,
		Hid
	} writemethod;

	void setWriteMethod();
	void initIRCamera();

	void write(unsigned char* output_report);
	void read(unsigned char* input_report);

	std::thread th;
	std::mutex mtx;
	static void updateThead(Wiimote*);
	void update();

public:
	struct {
		bool One, Two, A, B, Minus, Plus, Home, Up, Down, Left, Right;
	} Button;
	struct {
		bool LED1, LED2, LED3, LED4;
	} LEDs;
	bool Rumble;

	struct Acceletion {
		double x, y, z;
	} acc;

	//0 ÅÖ x, y ÅÖ 1 (1024x768)
	class Pointers {
	private:
		struct Pos {
			double x, y;
			int size;
		} pointers [4];

	public:
		Pointers() {
			for (int i = 0; i < 4; i++) 
				pointers[i].size = pointers[i].x = pointers[i].y = 0;
		}
		Pos& operator[](unsigned int n) {
			return pointers[n];
		}
		Pos getMaximunPos() {
			unsigned int size,num = 0;
			for (int i = 1; i < 4; i++)
				if (pointers[num].size < pointers[i].size && pointers[i].size == 0x0f)
					size = pointers[num = i].size;
			return pointers[num];
		}
	} pointer;

	Wiimote();
	~Wiimote();

	enum Status {
		Error = 0,
		Success = 1
	};

	Status connect();
	Status open();
	void close();

	void rumble(bool able);
	void ableRumble();
	void disableRumble();

	void setLED(bool first, bool second, bool third, bool fourth);
	//LED1:0x1 LED2:0x2, LED3: 0x4: LED4:0x8
	void setLED(unsigned char LED);
};