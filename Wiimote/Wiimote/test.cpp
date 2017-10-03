#include <iostream>
#include <iomanip>
#include "Wiimote.hpp"
#include <stdlib.h>
#include <Windows.h>
using namespace std;


auto main() -> int {
	Wiimote wii;
	wii.open();

	wii.setLED(0x01 | 0x02);

	cout << " ----------------------------" << endl;
	cout << "|       Wii Remote Test      |" << endl;
	cout << " ----------------------------\n" << endl;
	for (int i = 0; i < 2; i++) {
		wii.ableRumble();
		Sleep(50);
		wii.disableRumble();
		Sleep(50);
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
			<< fixed << setprecision(4) << wii.pointer.getMaximunPos().x
			<< " Y:"
			<< fixed << setprecision(4) << wii.pointer.getMaximunPos().y;
		Sleep(10);
	}
	wii.close();
	return 0;
}