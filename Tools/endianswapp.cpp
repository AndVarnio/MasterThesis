#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <map>
#include <tuple>

// Swapps the endianess for a cube with 16 bit values
int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cout << "Usage:\n"
                  << "endianswap input output\n\n"
                  << " in-format, out-format: BIP, BIL, BSQ\n";
        return -1;
    }

    const std::string infile_name(argv[1]);
    const std::string outfile_name(argv[2]);

    std::ifstream infile(infile_name, std::ifstream::in | std::ifstream::binary);
    std::ofstream outfile(outfile_name, std::ofstream::out | std::ofstream::binary);

    infile.seekg(0,std::ios::end);
    std::streampos size = infile.tellg();
    infile.seekg(0,std::ios::beg);

    std::cout << "Rearranging\n";
    while (!infile.eof()) {
        uint16_t value;
        infile.read((char*)&value, 2);
        uint16_t new_value =
            ((value & 0xFF00)>>8) | ((value & 0x00FF)<<8);
        outfile.write((char*)&new_value, 2);
    }


    std::cout << "Done.\n";
}
