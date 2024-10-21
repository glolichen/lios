// convert readpsf (https://github.com/talamus/rw-psf) generated plain text to integer bitmap for kernel

#include <iostream>
#include <fstream>
#include <string>

using ull = unsigned long long;
#define SET_BIT(num, pos) (num |= ((ull) 1) << (pos))

int main() {
	std::ifstream fin("vgain.txt");
	std::ofstream fout("vgaout.txt");
	std::string line;

	fout << "{\n";

	ull a, b;
	for (int i = 0; i < 256; i++) {
		// skip a line
		std::getline(fin, line);

		a = 0, b = 0;
		for (int j = 0; j < 8; j++) {
			std::getline(fin, line);
			for (int k = 0; k < 8; k++) {
				if (line[k] == 'X')
					SET_BIT(a, j * 8 + k);
			}
		}
		for (int j = 0; j < 8; j++) {
			std::getline(fin, line);
			for (int k = 0; k < 8; k++) {
				if (line[k] == 'X')
					SET_BIT(b, j * 8 + k);
			}
		}

		fout << std::hex << std::uppercase << "\t0x" << a << ", 0x" << b << ",\n";
	}

	fout << "}\n";

	fin.close();
	fout.close();

	return 0;
}
