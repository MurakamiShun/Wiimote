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
	///入力ボタン
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
	///振動
	///</summary>
	bool Rumble;
	///<summary>
	///加速度
	///</summary>
	struct Acceletion {
		double x, y, z;
	} acc;
	///<summary>
	///更新回数(flame/second)
	///</summary>
	double fps;

	//0 ≦ x, y ≦ 1 (1024x768)
	///<summary>
	///赤外線センサから取得した位置情報
	///</summary>
	class Pointers {
	public:
		///<summary>
		///位置情報と強度
		///</summary>
		struct Pos {
			double x, y;
			int size;
			Pos();
			Pos(const Pos& pos);
		};
		Pointers();
		///<summary>
		///0～4
		///</summary>
		Pos& operator[](unsigned int n);
		///<summary>
		///強度が最大の位置情報を返す
		///</summary>
		Pos getMaximumPos();
		///<summary>
		///センサーバーからの位置情報を返す
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
	///WiiリモコンをPCに接続する(未完成)
	///</summary>
	Status connect();
	///<summary>
	///Wiiリモコンを使える状態にする
	///</summary>
	Status open();
	///<summary>
	///openしたらclose
	///</summary>
	void close();

	///<summary>
	///振動のON・OFF
	///</summary>
	void rumble(bool able);
	
	///<summary>
	///振動ON
	///</summary>
	void ableRumble();
	///<summary>
	///振動OFF
	///</summary>
	void disableRumble();

	///<summary>
	///LEDを設定
	///</summary>
	void setLED(bool first, bool second, bool third, bool fourth);
	///<summary>
	///LED1:0x1, LED2:0x2, LED3:0x4, LED4:0x8
	///</summary>
	void setLED(unsigned char LED);
	///<summary>
	///IRカメラのモード設定
	///</summary>
	void initIRCamera(unsigned int mode);

	///<summary>
	///接続しているか
	///</summary>
	bool isOpened();
};