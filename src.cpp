#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <climits>
#include "queue.h"

#define oneMB 1024 * 1024 / sizeof(int)
#define halfKB 256 / sizeof(int)
#define eightNums 32 / sizeof(int)

#define chunk_size oneMB

struct chunkInfo {
    std::streampos posToRead;
    int numsLeft;
};

int partition(int* arr, int low, int high) {
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    int temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return (i + 1);
}

void quickSort(int* arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

void printFile(std::string fileName) {
    std::ifstream out;
    out.open(fileName, std::ios::binary);

    int numInts;
    out.read((char*)&numInts, sizeof(int));

    for(int i = 0; i < numInts; i++) {
        int x;
        out.read((char*)&x, sizeof(int));
        std::cout << x << " ";
    } 
    std::cout << std::endl;

    out.close();
}

void produceSortedChunks(std::ifstream& in, std::fstream& out, chunkInfo*& chunkInfoArr, int& numChunks) {
    int numInts;
    in.read((char*)&numInts, sizeof(int));
    out.write((char*)&numInts, sizeof(int));

    int chunkSize = chunk_size;
    int numFullChunks = numInts / chunkSize;
    int imbalance = (numInts % chunkSize == 0) ? 0 : 1;

    chunkInfoArr = new chunkInfo[numFullChunks + imbalance];

    for(int i = 0; i < numFullChunks; i++) {
        chunkInfoArr[i].posToRead = in.tellg();
        chunkInfoArr[i].numsLeft = chunkSize;
        int* chunk = new int[chunkSize];
        in.read((char*)chunk, chunkSize * sizeof(int));
        quickSort(chunk, 0, chunkSize - 1);
        out.write((char*)chunk, chunkSize * sizeof(int));
        delete[] chunk;
    }

    numChunks = numFullChunks + imbalance;

    if (imbalance == 1) {
        int remainingInts = numInts % chunkSize;
        chunkInfoArr[numChunks - imbalance].posToRead = in.tellg();
        chunkInfoArr[numChunks - imbalance].numsLeft = remainingInts;
        int* chunk = new int[remainingInts];
        in.read((char*)chunk, remainingInts * sizeof(int));
        quickSort(chunk, 0, remainingInts - 1);
        out.write((char*)chunk, remainingInts * sizeof(int));
        delete[] chunk;
    }

    in.close();
    out.close();
}

void generateBuffers (std::vector<Queue<int>>& buffers, chunkInfo*& chunkInfoArr, std::fstream& tempFile, int subchunkSize) {

    for (int i = 0; i < buffers.size(); i++) {

        if (buffers[i].empty() and chunkInfoArr[i].numsLeft > 0) {

            subchunkSize = std::min(subchunkSize, chunkInfoArr[i].numsLeft);
            int* subchunk = new int[subchunkSize];
            tempFile.seekg(chunkInfoArr[i].posToRead);
            tempFile.read((char*)subchunk, subchunkSize * sizeof(int));

            chunkInfoArr[i].posToRead += (subchunkSize * sizeof(int));
            chunkInfoArr[i].numsLeft -= subchunkSize;

            for(int j = 0; j < subchunkSize; j++)
                buffers[i].push_back(subchunk[j]);

            delete[] subchunk;
        }
    }
}

void mergeChunks(std::fstream& temp, std::ostream& out, chunkInfo*& chunkInfoArr, int numChunks) {
    int numInts;
    temp.seekg(0);
    temp.read((char*)&numInts, sizeof(int));
    out.write((char*)&numInts, sizeof(int));

    std::streampos curr;

    int subchunkSize = numInts / chunk_size + 1;

    std::vector<Queue<int>> buffers(numChunks);
    generateBuffers(buffers, chunkInfoArr, temp, subchunkSize);

    int counter = 0;
    
    int* smallestValues = new int[subchunkSize];

    while (true) {
        bool everythingEmpty = true;

        int minValue = INT_MAX;
        int minValueIndex = 0;

        for (int bufferNo = 0; bufferNo < buffers.size(); bufferNo++) {
            if (!buffers[bufferNo].empty() and buffers[bufferNo].front() < minValue) {
                minValue = buffers[bufferNo].front();
                minValueIndex = bufferNo;
            }
        }

        smallestValues[counter++] = minValue;
        buffers[minValueIndex].pop_front();

        if (buffers[minValueIndex].empty())
            generateBuffers(buffers, chunkInfoArr, temp, subchunkSize);

        for (int i = 0; i < numChunks; i++) {
            if (chunkInfoArr[i].numsLeft > 0 or !buffers[i].empty()) {
                everythingEmpty = false;
                break;
            }
        }

        if (counter == subchunkSize or everythingEmpty) {
            out.write((char*)smallestValues, counter * sizeof(int));
            counter = 0;
        }

        if (everythingEmpty)
            break;
    }

    temp.close();
}

int main(int argc, char** argv) {

    // std::ios::ios_base::sync_with_stdio(0);
    // std::cout.tie(NULL);
    // std::cin.tie(NULL);

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << "<file_name>\n";
        return 1;
    }

    std::string fileName = argv[1];
    std::string tempFileName = "temp-" + fileName;
    std::string newFileName = "sorted-" + fileName;

    std::ofstream fileOut;
    std::fstream fileTemp;
    std::ifstream fileIn;

    chunkInfo* chunkInfoArr;
    int numChunks; //functions as size for chunkPositions

    fileIn.open(fileName, std::ios::binary);
    fileTemp.open(tempFileName, std::ios::binary | std::ios::out);
    produceSortedChunks(fileIn, fileTemp, chunkInfoArr, numChunks);

    fileTemp.open(tempFileName, std::ios::binary | std::ios::in);
    fileOut.open(newFileName, std::ios::binary);
    mergeChunks(fileTemp, fileOut, chunkInfoArr, numChunks);

    fileOut.close();
    printFile(newFileName);

    delete[] chunkInfoArr;
    return 0;
}
