#include "Wiimote.hpp"
#include <Windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <chrono>

#include <thread>

#pragma comment(lib,"hid")
#pragma comment(lib, "setupapi")



enum OutputReport {
	LEDs = 0x11,
	DataReportType = 0x12,
	IR = 0x13,
	Status = 0x15,
	WriteMemory = 0x16,
	ReadMemory = 0x17,
	IR2 = 0x1a
};


const unsigned char IRblock[5][11] = {
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0x64,0x00,0xfe, 0xfd,0x05 },
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0x96,0x00,0xb4, 0xb3,0x04},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0xaa,0x00,0x64, 0x63,0x03},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0xc8,0x00,0x36, 0x35,0x03},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0x72,0x00,0x20, 0x1f,0x03}
};

Wiimote::Wiimote() {
	wiihandle = nullptr;
	Button.One = Button.Two = Button.A = Button.B = 
	Button.Minus = Button.Plus = Button.Home = 
	Button.Up = Button.Down = Button.Left = Button.Right = false;
	acc.x = acc.y = acc.z = 0;
}

Wiimote::~Wiimote() {
	if (wiihandle != nullptr) {
		close();
		th.join();
	}
}

void Wiimote::setWriteMethod() {
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];
	if (!HidD_SetOutputReport(wiihandle, out, output_length)) {
		writemethod = WriteMethod::Hid;
	}
	else {
		writemethod = WriteMethod::File;
		DWORD buff;
		WriteFile(
			(HANDLE)wiihandle,
			out,
			output_length,
			&buff,
			NULL
		);
	}
	HidD_GetInputReport(wiihandle, in, input_length);
	if (in[0] == 0x20)
		int j = in[0];


	delete[] out;
	delete[] in;
	return;
}


void Wiimote::write(unsigned char* output_report) {
	if (writemethod == WriteMethod::File) {
		DWORD buff;
		WriteFile(
			(HANDLE)wiihandle,
			output_report,
			output_length,
			&buff,
			NULL
		);
	}
	else {
		HidD_SetOutputReport(wiihandle, output_report, output_length);
	}
}

void Wiimote::read(unsigned char * input_report) {
	DWORD buff;
	ReadFile(
		(HANDLE)wiihandle,
		input_report,
		input_length,
		&buff,
		(LPOVERLAPPED)NULL
	);
}

Wiimote::Status Wiimote::connect() {
	return Status();
}

Wiimote::Status Wiimote::open() {
	//GUIDを取得
	GUID HidGuid;
	HidD_GetHidGuid(&HidGuid);

	//全てのHIDの情報セットを取得
	HDEVINFO hDevInfo;
	hDevInfo = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE);

	//HIDを列挙
	SP_DEVICE_INTERFACE_DATA DevData;
	DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	for (int index = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &HidGuid, index, &DevData); index++) {
		//詳細を取得

		//データサイズの取得
		unsigned long size;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevData, NULL, 0, &size, NULL);
		//メモリの確保
		PSP_INTERFACE_DEVICE_DETAIL_DATA Detail;
		Detail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)new char[size];
		Detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

		//確保したメモリ領域に情報を取得
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevData, Detail, size, &size, 0);

		//とりあえずハンドルを取得
		HANDLE handle = CreateFile(
			Detail->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			(LPSECURITY_ATTRIBUTES)NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
		delete[] Detail;
		//エラー処理
		if (handle == INVALID_HANDLE_VALUE) {
			continue;
		}

		//アトリビュートを取得
		HIDD_ATTRIBUTES attr;
		attr.Size = sizeof(attr);
		if (HidD_GetAttributes(handle, &attr)) {
			//VIDとPIDを比較してwiiリモコンかどうかを調べる
			if (attr.VendorID == VID && attr.ProductID == PID) {
				//Capsの取得
				HIDP_CAPS Capabilities;
				PHIDP_PREPARSED_DATA PreparsedData;
				NTSTATUS Result = HIDP_STATUS_INVALID_PREPARSED_DATA;

				if (HidD_GetPreparsedData(handle, &PreparsedData)) {
					Result = HidP_GetCaps(PreparsedData, &Capabilities);
					HidD_FreePreparsedData(PreparsedData);
				}
				//Capsを取得できたか確認
				if (Result == HIDP_STATUS_SUCCESS) {
					//接続成功
					input_length = Capabilities.InputReportByteLength;
					output_length = Capabilities.OutputReportByteLength;
					wiihandle = handle;
					Sleep(100);
					setWriteMethod();
					th = std::thread(updateThead, this);
					exit_init = false;
					while (!exit_init)
						Sleep(10);
					return Status::Success;
				}
			}
		}
		//使わなかったハンドルをクローズ
		CloseHandle(handle);
	}
	return Status::Error;
}

void Wiimote::close() {
	if (wiihandle != nullptr) {
		CloseHandle(wiihandle);
		wiihandle = nullptr;
		th.join();
	}
}

void Wiimote::rumble(bool on){
	Rumble = on;
}

void Wiimote::ableRumble(){
	rumble(true);
}

void Wiimote::disableRumble(){
	rumble(false);
}


void Wiimote::setLED(bool first, bool second, bool third, bool fourth) {
	LEDs.LED1 = first;
	LEDs.LED2 = second;
	LEDs.LED3 = third;
	LEDs.LED4 = fourth;
}

void Wiimote::setLED(unsigned char LED) {
	setLED(
		(LED & 0x01) ? true : false,
		(LED & 0x02) ? true : false,
		(LED & 0x04) ? true : false,
		(LED & 0x08) ? true : false
	);
}

void Wiimote::update() {
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	out[0] = OutputReport::LEDs;
	out[1] =
		(LEDs.LED1 ? 0x10 : 0) |
		(LEDs.LED2 ? 0x20 : 0) |
		(LEDs.LED3 ? 0x40 : 0) |
		(LEDs.LED4 ? 0x80 : 0)
		| (Rumble ? 0x01 : 0);

	write(out);
	read(in);

	//読み込み
	read(in);
	//判定
	//0x33
	//ボタン
	Button.One = (in[2] & 0x0002) ? true : false;
	Button.Two = (in[2] & 0x0001) ? true : false;
	Button.A = (in[2] & 0x0008) ? true : false;
	Button.B = (in[2] & 0x0004) ? true : false;
	Button.Minus = (in[2] & 0x0010) ? true : false;
	Button.Plus = (in[1] & 0x0010) ? true : false;
	Button.Home = (in[2] & 0x0080) ? true : false;
	Button.Up = (in[1] & 0x0008) ? true : false;
	Button.Down = (in[1] & 0x0004) ? true : false;
	Button.Left = (in[1] & 0x0001) ? true : false;
	Button.Right = (in[1] & 0x0002) ? true : false;
	
	//加速度センサ
	const int zero = 481;
	unsigned int x = ((in[1] & (0x20 | 0x40)) >> 5) + (in[3] << 2);
	unsigned int y = ((in[2] & 0x20) >> 4) + (in[4] << 2);
	unsigned int z = ((in[2] & 0x40) >> 5) + (in[5] << 2);
	acc.x = ((double)((int)x - zero)) / 98.0;
	acc.y = ((double)((int)y - zero)) / 98.0;
	acc.z = ((double)((int)z - zero)) / 98.0;
	
	//IRカメラ
	for (int i = 0; i < 4; i++) {
		x = in[i * 3 + 6] + ((unsigned int)(in[i * 3 + 8] & 0x30) << 4);
		y = in[i * 3 + 7] + ((unsigned int)(in[i * 3 + 8] & 0xC0) << 2);
		unsigned short size = in[i * 3 + 8] & 0xf;
		pointer[i].x = (double)x / 1023;
		pointer[i].y = (double)y / 767;
		pointer[i].size = size;
		if (size == 15) {
			pointer[i].size = -1;
		}
	}
	delete[] out;
	delete[] in;
}

void Wiimote::updateThead(Wiimote* wii) {
	unsigned char* out = new unsigned char[wii->output_length];
	unsigned char* in = new unsigned char[wii->input_length];
	//データ転送モードの設定
	out[0] = OutputReport::DataReportType;
	out[1] = 0x00;
	out[2] = 0x33;
	
	wii->write(out);
	wii->read(in);
	
	//デフォ3
	wii->initIRCamera(3);
	
	wii->setLED(0x01);
	delete[] out;
	delete[] in;

	wii->exit_init = true;

	while (wii->wiihandle != nullptr) {
		auto start = std::chrono::system_clock::now();
		wii->update();
		Sleep(2);
		wii->fps = 1000.0/ (double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
	}
}

void Wiimote::initIRCamera(unsigned int mode) {
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];
	
	mtx.lock();
	//IRカメラの有効化
	out[0] = OutputReport::IR;
	out[1] = 0x04;
	write(out);
	read(in);

	out[0] = OutputReport::IR2;
	out[1] = 0x04;
	write(out);
	read(in);
	Sleep(60);

	//0xb00030に0x01を書き込む
	out[0] = OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x30;
	//バイト数
	out[5] = 1;
	//データ(max16byte)
	out[6] = 0x08;
	write(out);
	read(in);
	Sleep(60);


	//block1
	out[0] = OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x00;
	//バイト数
	out[5] = 9;
	//データ(max16byte)
	out[6] = IRblock[mode][0];
	out[7] = IRblock[mode][1];
	out[8] = IRblock[mode][2];
	out[9] = IRblock[mode][3];
	out[10] = IRblock[mode][4];
	out[11] = IRblock[mode][5];
	out[12] = IRblock[mode][6];
	out[13] = IRblock[mode][7];
	out[14] = IRblock[mode][8];
	write(out);
	read(in);
	Sleep(60);

	//block2
	out[0] = OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x1a;
	//バイト数
	out[5] = 2;
	//データ(max16byte)
	out[6] = IRblock[mode][9];
	out[7] = IRblock[mode][10];
	write(out);
	read(in);
	Sleep(60);

	//mode number
	out[0] = OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x33;
	//バイト数
	out[5] = 1;
	//データ(max16byte)
	out[6] = 3;
	write(out);
	read(in);
	Sleep(60);

	//0xb00030に0x08を書き込む
	out[0] = OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x30;
	//バイト数
	out[5] = 1;
	//データ(max16byte)
	out[6] = 0x08;
	write(out);
	read(in);
	Sleep(60);

	mtx.unlock();
	delete[] out;
	delete[] in;
}

Wiimote::Pointers::Pos Wiimote::Pointers::getMaximumPos() {
	unsigned int num = 0;
	for (int i = 1; i < 4; i++){
		if (pointers[i].size != -1 && pointers[num].size > pointers[i].size) {
			num = i;
		}
	}
	return pointers[num];
}

Wiimote::Pointers::Pos Wiimote::Pointers::getBarPos() {
	Pos pos;
	//1>2
	unsigned int num1 = 0, num2 = 0;
	for (int i = 1; i < 4; i++) {
		if (pointers[i].size != -1 && pointers[num1].size >= pointers[i].size) {
			num2 = num1;
			num1 = i;
		}
	}
	pos.x = (pointers[num1].x + pointers[num2].x) / 2;
	pos.y = (pointers[num1].y + pointers[num2].y) / 2;
	pos.size = (pointers[num1].size + pointers[num2].size) / 2;
	return pos;
}

Wiimote::Pointers::Pointers() {
	for (int i = 0; i < 4; i++) {
		pointers[i].size = 0;
		pointers[i].x = pointers[i].y = 0.0;
	}
}

Wiimote::Pointers::Pos& Wiimote::Pointers::operator[](unsigned int n) {
	return pointers[n];
}

bool Wiimote::isOpened() {
	if (wiihandle == nullptr)
		return false;
	else
		return true;
}



Wiimote::Pointers::Pos::Pos() {
	x = y = -1;
	size = -1;
}

Wiimote::Pointers::Pos::Pos(const Pos& pos) {
	x = pos.x;
	y = pos.y;
	size = pos.size;
}