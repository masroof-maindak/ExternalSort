#include <iostream>
#include <fstream>
#include <random>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <file_name> <IntendedFileSize (in MBs)>\n";
    }

    int fileSize = atoi(argv[2]);
    int numInts = (fileSize * 1024 * 1024) / sizeof(int);

    std::string fileName = argv[1];
    std::ofstream file(fileName, std::ios::binary);
    file.write((char*)&numInts, sizeof(int));

    if (file.is_open()) {
        for (int i = 0; i < numInts; i++) {
            int randNo = rand() % 10000000;
            file.write((char*)&randNo, sizeof(int));
        }
    }

    file.close();
    return 0;
}