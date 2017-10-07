#include <iostream>
#include <iomanip>
#include "Wiimote.hpp"
#include <stdlib.h>
#include <Windows.h>
using namespace std;


auto main() -> int {
	Wiimote wii;
	if (wii.open() == Wiimote::Status::Error) {
		return 0;
	}

	wii.setLED(0x01 | 0x02);

	cout << " ----------------------------" << endl;
	cout << "|       Wii Remote Test      |" << endl;
	cout << " ----------------------------\n" << endl;
	for (int i = 0; i < 2; i++) {
		wii.ableRumble();
		Sleep(100);
		wii.disableRumble();
		Sleep(100);
	}
	cout << "Homeで終了" << endl;
	while (!wii.Button.Home) {
		cout << "\rA:" << wii.Button.A
			<< " B:" << wii.Button.B
			<< " +:" << wii.Button.Plus
			<< " -:" << wii.Button.Minus
			<< " 1:" << wii.Button.One
			<< " 2:" << wii.Button.Two
			<< " ↑:" << wii.Button.Up
			<< " ↓:" << wii.Button.Down
			<< " ←:" << wii.Button.Left
			<< " →:" << wii.Button.Right
			<< " X:"
			<< fixed << setprecision(4) << wii.pointer.getBarPos().x
			<< " Y:"
			<< fixed << setprecision(4) << wii.pointer.getBarPos().y
			<< "  "
			<< setw(3) << right << (int)wii.fps << " fps";
		Sleep(30);
	}
	wii.close();
	return 0;
}