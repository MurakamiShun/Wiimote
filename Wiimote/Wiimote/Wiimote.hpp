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

	void write(unsigned char* output_report);
	void read(unsigned char* input_report);

	std::thread th;
	std::mutex mtx;
	static void updateThead(Wiimote*);
	void update();

	bool exit_init;

public:
	///<summary>
	///���̓{�^��
	///</summary>
	struct {
		bool One, Two, A, B, Minus, Plus, Home, Up, Down, Left, Right;
	} Button;
	///<summary>
	///LED
	///</summary>
	struct {
		bool LED1, LED2, LED3, LED4;
	} LEDs;
	///<summary>
	///�U��
	///</summary>
	bool Rumble;
	///<summary>
	///�����x
	///</summary>
	struct Acceletion {
		double x, y, z;
	} acc;
	///<summary>
	///�X�V��(flame/second)
	///</summary>
	double fps;

	//0 �� x, y �� 1 (1024x768)
	///<summary>
	///�ԊO���Z���T����擾�����ʒu���
	///</summary>
	class Pointers {
	public:
		///<summary>
		///�ʒu���Ƌ��x
		///</summary>
		struct Pos {
			double x, y;
			int size;
			Pos();
			Pos(const Pos& pos);
		};
		Pointers();
		///<summary>
		///0�`4
		///</summary>
		Pos& operator[](unsigned int n);
		///<summary>
		///���x���ő�̈ʒu����Ԃ�
		///</summary>
		Pos getMaximumPos();
		///<summary>
		///�Z���T�[�o�[����̈ʒu����Ԃ�
		///</summary>
		Pos getBarPos();
	private:
		Pos pointers [4];
	} pointer;

	Wiimote();
	~Wiimote();

	enum Status {
		Error = 0,
		Success = 1
	};

	///<summary>
	///Wii�����R����PC�ɐڑ�����(������)
	///</summary>
	Status connect();
	///<summary>
	///Wii�����R�����g�����Ԃɂ���
	///</summary>
	Status open();
	///<summary>
	///open������close
	///</summary>
	void close();

	///<summary>
	///�U����ON�EOFF
	///</summary>
	void rumble(bool able);
	
	///<summary>
	///�U��ON
	///</summary>
	void ableRumble();
	///<summary>
	///�U��OFF
	///</summary>
	void disableRumble();

	///<summary>
	///LED��ݒ�
	///</summary>
	void setLED(bool first, bool second, bool third, bool fourth);
	///<summary>
	///LED1:0x1, LED2:0x2, LED3:0x4, LED4:0x8
	///</summary>
	void setLED(unsigned char LED);
	///<summary>
	///IR�J�����̃��[�h�ݒ�
	///</summary>
	void initIRCamera(unsigned int mode);

	///<summary>
	///�ڑ����Ă��邩
	///</summary>
	bool isOpened();
};