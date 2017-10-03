#include "Wiimote.hpp"
#include <Windows.h>
#include <setupapi.h>
#include <hidsdi.h>

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
	bool error = ReadFile(
		(HANDLE)wiihandle,
		input_report,
		input_length,
		&buff,
		NULL
	);
}

Wiimote::Status Wiimote::connect() {
	return Status();
}

Wiimote::Status Wiimote::open() {
	//GUID���擾
	GUID HidGuid;
	HidD_GetHidGuid(&HidGuid);

	//�S�Ă�HID�̏��Z�b�g���擾
	HDEVINFO hDevInfo;
	hDevInfo = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE);

	//HID���
	SP_DEVICE_INTERFACE_DATA DevData;
	DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	for (int index = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &HidGuid, index, &DevData); index++) {
		//�ڍׂ��擾

		//�f�[�^�T�C�Y�̎擾
		unsigned long size;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevData, NULL, 0, &size, NULL);
		//�������̊m��
		PSP_INTERFACE_DEVICE_DETAIL_DATA Detail;
		Detail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)new char[size];
		Detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

		//�m�ۂ����������̈�ɏ����擾
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevData, Detail, size, &size, 0);

		//�Ƃ肠�����n���h�����擾
		HANDLE handle = CreateFile(
			Detail->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			(LPSECURITY_ATTRIBUTES)NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
		//�G���[����
		if (handle == INVALID_HANDLE_VALUE) {
			continue;
		}

		//�A�g���r���[�g���擾
		HIDD_ATTRIBUTES attr;
		attr.Size = sizeof(attr);
		if (HidD_GetAttributes(handle, &attr)) {
			//VID��PID���r����wii�����R�����ǂ����𒲂ׂ�
			if (attr.VendorID == VID && attr.ProductID == PID) {
				//Caps�̎擾
				HIDP_CAPS Capabilities;
				PHIDP_PREPARSED_DATA PreparsedData;
				NTSTATUS Result = HIDP_STATUS_INVALID_PREPARSED_DATA;

				if (HidD_GetPreparsedData(handle, &PreparsedData)) {
					Result = HidP_GetCaps(PreparsedData, &Capabilities);
					HidD_FreePreparsedData(PreparsedData);
				}
				//Caps���擾�ł������m�F
				if (Result == HIDP_STATUS_SUCCESS) {
					//�ڑ�����
					input_length = Capabilities.InputReportByteLength;
					output_length = Capabilities.OutputReportByteLength;
					wiihandle = handle;
					Sleep(100);
					setWriteMethod();
					th = std::thread(updateThead, this);
					return Status::Success;
				}
			}
		}
		//�g��Ȃ������n���h�����N���[�Y
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
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	Rumble = on;

	out[0] = OutputReport::LEDs;
	out[1] =
		(LEDs.LED1 ? 0x10 : 0) |
		(LEDs.LED2 ? 0x20 : 0) |
		(LEDs.LED3 ? 0x40 : 0) |
		(LEDs.LED4 ? 0x80 : 0)
		| Rumble;

	mtx.lock();
	write(out);
	read(in);
	mtx.unlock();

	delete[] out;
	delete[] in;
}

void Wiimote::ableRumble(){
	rumble(true);
}

void Wiimote::disableRumble(){
	rumble(false);
}


void Wiimote::setLED(bool first, bool second, bool third, bool fourth) {
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];
	LEDs.LED1 = first;
	LEDs.LED2 = second;
	LEDs.LED3 = third;
	LEDs.LED4 = fourth;

	out[0] = OutputReport::LEDs;
	out[1] = 
		(first  ? 0x10 : 0) |
		(second ? 0x20 : 0) |
		(third  ? 0x40 : 0) |
		(fourth ? 0x80 : 0)
		| Rumble;

	mtx.lock();
	write(out);
	read(in);
	mtx.unlock();

	delete[] out;
	delete[] in;
}

void Wiimote::setLED(unsigned char LED) {
	setLED(
		LED & 0x01,
		LED & 0x02,
		LED & 0x04,
		LED & 0x08
	);
}

void Wiimote::update() {
	if (wiihandle == nullptr)
		return;

	unsigned char* in = new unsigned char[input_length];

	mtx.lock();
	//�ǂݍ���
	read(in);
	mtx.unlock();
	//����
	//0x33
	//�{�^��
	Button.One = in[2] & 0x0002;
	Button.Two = in[2] & 0x0001;
	Button.A = in[2] & 0x0008;
	Button.B = in[2] & 0x0004;
	Button.Minus = in[2] & 0x0010;
	Button.Plus = in[1] & 0x0010;
	Button.Home = in[2] & 0x0080;
	Button.Up = in[1] & 0x0008;
	Button.Down = in[1] & 0x0004;
	Button.Left = in[1] & 0x0001;
	Button.Right = in[1] & 0x0002;
	
	//�����x�Z���T
	const int zero = 481;
	unsigned int x = ((in[1] & (0x20 | 0x40)) >> 5) + (in[3] << 2);
	unsigned int y = ((in[2] & 0x20) >> 4) + (in[4] << 2);
	unsigned int z = ((in[2] & 0x40) >> 5) + (in[5] << 2);
	acc.x = ((double)((int)x - zero)) / 98.0;
	acc.y = ((double)((int)y - zero)) / 98.0;
	acc.z = ((double)((int)z - zero)) / 98.0;
	
	//IR�J����
	for (int i = 0; i < 4; i++) {
		x = in[i * 3 + 6] + ((unsigned int)(in[i * 3 + 8] & 0x30) << 4);
		y = in[i * 3 + 7] + ((unsigned int)(in[i * 3 + 8] & 0xC0) << 2);
		unsigned short size = in[i * 3 + 8] & 0xf;
		pointer[i].x = (double)x / 1023;
		pointer[i].y = (double)y / 767;
		pointer[i].size = size;
		if (size == 15) {
			pointer[i].x = -1;
			pointer[i].y = -1;
			pointer[i].size = -1;
		}
	}
	delete[] in;
}

void Wiimote::updateThead(Wiimote* wii) {
	unsigned char* out = new unsigned char[wii->output_length];
	unsigned char* in = new unsigned char[wii->input_length];
	wii->setLED(0x01);
	//�f�[�^�]�����[�h�̐ݒ�
	out[0] = OutputReport::DataReportType;
	out[1] = 0x00;
	out[2] = 0x33;
	wii->mtx.lock();
	wii->write(out);
	wii->read(in);
	wii->mtx.unlock();
	wii->initIRCamera();

	delete[] out;
	delete[] in;

	while (wii->wiihandle != nullptr) {
		wii->update();
		//Sleep(1);
	}
}

void Wiimote::initIRCamera() {
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];
	
	mtx.lock();
	//IR�J�����̗L����
	out[0] = OutputReport::IR;
	out[1] = 0x04;
	write(out);
	read(in);

	out[0] = OutputReport::IR2;
	out[1] = 0x04;
	write(out);
	read(in);
	Sleep(60);

	//0xb00030��0x01����������
	out[0] = OutputReport::WriteMemory;
	//���W�X�^����
	out[1] = 0x04;
	//�A�h���X
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x30;
	//�o�C�g��
	out[5] = 1;
	//�f�[�^(max16byte)
	out[6] = 0x08;
	write(out);
	//read(in);
	Sleep(60);



	//block1
	out[0] = OutputReport::WriteMemory;
	//���W�X�^����
	out[1] = 0x04;
	//�A�h���X
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x00;
	//�o�C�g��
	out[5] = 9;
	//�f�[�^(max16byte)
	out[6] = 0x00;
	out[7] = 0x00;
	out[8] = 0x00;
	out[9] = 0x00;
	out[10] = 0x00;
	out[11] = 0x00;
	out[12] = 0x90;
	out[13] = 0x00;
	out[14] = 0xC0;
	write(out);
	//read(in);
	Sleep(60);

	//block2
	out[0] = OutputReport::WriteMemory;
	//���W�X�^����
	out[1] = 0x04;
	//�A�h���X
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x1a;
	//�o�C�g��
	out[5] = 2;
	//�f�[�^(max16byte)
	out[6] = 0x40;
	out[7] = 0x00;
	write(out);
	//read(in);
	Sleep(60);

	//mode number
	out[0] = OutputReport::WriteMemory;
	//���W�X�^����
	out[1] = 0x04;
	//�A�h���X
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x33;
	//�o�C�g��
	out[5] = 1;
	//�f�[�^(max16byte)
	out[6] = 3;
	write(out);
	//read(in);
	Sleep(60);

	//0xb00030��0x08����������
	out[0] = OutputReport::WriteMemory;
	//���W�X�^����
	out[1] = 0x04;
	//�A�h���X
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x30;
	//�o�C�g��
	out[5] = 1;
	//�f�[�^(max16byte)
	out[6] = 0x08;
	write(out);
	//read(in);
	Sleep(60);

	mtx.unlock();
}