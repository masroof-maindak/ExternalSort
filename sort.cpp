#include <iostream>
#include <fstream>
#include <vector>
#include <climits>
#include <queue>
#include <utility>
#include "queue.h"

#define oneMB 1024 * 1024 / sizeof(int)
#define halfKB 256 / sizeof(int)
#define eightNums 32 / sizeof(int)

#define chunk_size oneMB

struct chunkInfo {
    std::streampos posToRead;   // WHERE this chunk begins
    int numsLeft;               // HOW MANY integers are left in this chunk
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
    int numInts = 0;
    in.read((char*)&numInts, sizeof(int));
    out.write((char*)&numInts, sizeof(int));

    int chunkSize = chunk_size;

    // TODO: Make this concise
    int numFullChunks = numInts / chunkSize;
    int imbalance = (numInts % chunkSize == 0) ? 0 : 1;
    numChunks = numFullChunks + imbalance;
    
    chunkInfoArr = new chunkInfo[numChunks];

    /*
    for(int i = 0; i < numFullChunks; i++) {
        chunkInfoArr[i].posToRead = in.tellg();
        chunkInfoArr[i].numsLeft = chunkSize;
        int* chunk = new int[chunkSize];
        in.read((char*)chunk, chunkSize * sizeof(int));
        quickSort(chunk, 0, chunkSize - 1);
        out.write((char*)chunk, chunkSize * sizeof(int));
        delete[] chunk;
    }

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
    */
        
    // TEST: Combined both blocks above into one
    int remainingInts = numInts;
    for(int i = 0; i < numChunks; i++) {
        int thisChunksSize = std::min(chunkSize, remainingInts);
        chunkInfoArr[i].posToRead = in.tellg();
        chunkInfoArr[i].numsLeft = thisChunksSize;
        int* chunk = new int[thisChunksSize];
        in.read((char*)chunk, thisChunksSize * sizeof(int));
        quickSort(chunk, 0, thisChunksSize - 1);
        out.write((char*)chunk, thisChunksSize * sizeof(int));
        delete[] chunk;
        remainingInts -= thisChunksSize;
    }

    in.close();
    out.close();
}


void generateBuffers (std::vector<Queue<int>>& buffers, chunkInfo*& chunkInfoArr, std::fstream& tempFile, int subchunkSize) {
    // If a buffer is empty and has numbers left in its corresponding chunk, then grab
    // 'as much as you can' (subchunk or less) from that chunk and put it in the buffer
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

    int subchunkSize = numInts / chunk_size + 1;

    std::vector<Queue<int>> buffers(numChunks);
    generateBuffers (buffers, chunkInfoArr, temp, subchunkSize);

    int counter = 0;
    int* smallestValues = new int[subchunkSize];
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> smallest;

    // TEST: Alternate implementation using priority queue
    for (int i = 0; i < buffers.size(); i++)
        if (!buffers[i].empty())
            smallest.push(std::make_pair(buffers[i].front(), i));

    while (!smallest.empty()) {
        // 1. Get the smallest value across all the smallest values
        std::pair<int,int> minPair = smallest.top();
        smallest.pop();
        int minIndex = minIndex;

        // 2. Push it to an array of the smallest values, and remove it from its respective queue
        smallestValues[counter++] = minPair.first;
        buffers[minIndex].pop_front();

        // 3. If that buffer is now empty, but has more integers to read, read them
        if (buffers[minIndex].empty() and chunkInfoArr[minIndex].numsLeft > 0) {
            int valuesToRead = std::min(subchunkSize, chunkInfoArr[minIndex].numsLeft);
            int* subchunk = new int[valuesToRead];

            // Navigate to where that chunk begins and read the next 'valuesToRead' integers
            temp.seekg(chunkInfoArr[minIndex].posToRead);
            temp.read((char*)subchunk, valuesToRead * sizeof(int));

            // Update chunk info
            chunkInfoArr[minIndex].posToRead += (valuesToRead * sizeof(int));
            chunkInfoArr[minIndex].numsLeft -= valuesToRead;

            // Push the read integers to the buffer
            for(int j = 0; j < valuesToRead; j++)
                buffers[minIndex].push_back(subchunk[j]);

            delete[] subchunk;
        }

        // 3.5. If the buffer is (now) not empty, push its smallest value (i.e first) to the minheap
            if (!buffers[minIndex].empty())
                smallest.push(std::make_pair(buffers[minIndex].front(), minIndex));

        // 5. If the array of smallest values is full, write to file
        if (counter == subchunkSize) {
            out.write((char*)smallestValues, counter * sizeof(int));
            counter = 0;
        }
    }

    // Write what's left
    out.write((char*)smallestValues, counter * sizeof(int));
    delete[] smallestValues;

    // Original implementation
    /*
    while (true) {
        bool everythingEmpty = true;
        int minValue = INT_MAX;
        int minValueIndex = 0;

        // 1. Get the smallest value from each chunk's queue
        for (int i = 0; i < buffers.size(); i++) {
            if (!buffers[i].empty() and buffers[i].front() < minValue) {
                minValue = buffers[i].front();
                minValueIndex = i;
            }
        }

        // TODO: Instead of manual searching, replace the above with a priority queue?
        // We would need to make it a queue of pair<int,int> though so we can also store
        // the index of the buffer where that value came from

        // 2. Push the min value to an array of the smallest values, and remove it from its queue
        smallestValues[counter++] = minValue;
        buffers[minValueIndex].pop_front();

        // 3. If removing the smallest value made a chunk's queue (buffer) go empty, 'replenish'
        if (buffers[minValueIndex].empty())
            generateBuffers(buffers, chunkInfoArr, temp, subchunkSize);

        // 4. Check if a single empty buffer exists
        for (int i = 0; i < numChunks; i++) {
            if (chunkInfoArr[i].numsLeft > 0 or !buffers[i].empty()) {
                everythingEmpty = false;
                break;
            }
        }
    
        // 5. If the array of smallest values is full or we have emptied the buffers, write to file
        if (counter == subchunkSize or everythingEmpty) {
            out.write((char*)smallestValues, counter * sizeof(int));
            counter = 0;
        }
    
        // 6. If all the buffers are empty, we are bound to have written them as per above, so exit
        if (everythingEmpty)
            break;
    }
    */

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
