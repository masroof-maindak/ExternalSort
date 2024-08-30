#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <queue>
#include <vector>

#define oneMB	  1024 * 1024 / sizeof(std::uint32_t)
#define halfKB	  256 / sizeof(std::uint32_t)
#define eightNums 32 / sizeof(std::uint32_t)
const int chunk_size = oneMB * 10;

struct chunkInfo {
	std::streampos posToRead; // Where this chunk begins
	int numsLeft;			  // How many integers are left in this chunk
};

struct bufferVal {
	int val;
	int chunkIndex;

	bool operator>(const bufferVal &other) const { return val > other.val; }
};

void printFile(std::string fileName) {
	std::ifstream out(fileName, std::ios::binary);
	int numInts = 0;
	out.read((char *)&numInts, sizeof(int));

	int remainingInts = numInts;
	while (remainingInts > 0) {
		int currChunkSize = std::min(chunk_size, remainingInts);

		std::vector<int> chunk(currChunkSize);
		out.read((char *)chunk.data(), currChunkSize * sizeof(int));

		for (int j = 0; j < currChunkSize; j++)
			std::cout << chunk[j] << " ";
		remainingInts -= currChunkSize;
	}

	std::cout << "\n";

	out.close();
}

int produceSortedChunks(std::ifstream &in, std::fstream &out,
						std::vector<chunkInfo> &chunkInfoArr) {
	int numInts = 0;
	in.read((char *)&numInts, sizeof(int));
	out.write((char *)&numInts, sizeof(int));

	int numFullChunks = numInts / chunk_size;
	int imbalance	  = (numInts % chunk_size == 0) ? 0 : 1;
	int numChunks	  = numFullChunks + imbalance;

	chunkInfoArr.resize(numChunks);

	int remainingInts = numInts;
	for (int i = 0; i < numChunks; i++) {
		int thisChunksSize		  = std::min(chunk_size, remainingInts);
		chunkInfoArr[i].posToRead = in.tellg();
		chunkInfoArr[i].numsLeft  = thisChunksSize;

		std::vector<int> chunk(thisChunksSize);
		in.read((char *)chunk.data(), thisChunksSize * sizeof(int));
		std::sort(chunk.begin(), chunk.end());
		out.write((char *)chunk.data(), thisChunksSize * sizeof(int));

		remainingInts -= thisChunksSize;
	}

	return numChunks;
}

void replenishBuffer(std::vector<std::queue<int>> &buffers,
					 std::vector<chunkInfo> &chunkInfoArr,
					 std::fstream &tempFile, int subchunkSize, int chunkIndex) {

	subchunkSize = std::min(subchunkSize, chunkInfoArr[chunkIndex].numsLeft);
	std::vector<int> subchunk(subchunkSize);

	tempFile.seekg(chunkInfoArr[chunkIndex].posToRead);
	tempFile.read((char *)subchunk.data(), subchunkSize * sizeof(int));

	chunkInfoArr[chunkIndex].posToRead += (subchunkSize * sizeof(int));
	chunkInfoArr[chunkIndex].numsLeft -= subchunkSize;

	for (int j = 0; j < subchunkSize; j++)
		buffers[chunkIndex].push(subchunk[j]);
}

void mergeSortedChunks(std::fstream &temp, std::ostream &out,
					   std::vector<chunkInfo> &chunkInfoArr, int numChunks) {
	int numInts;
	temp.seekg(0);
	temp.read((char *)&numInts, sizeof(int));
	out.write((char *)&numInts, sizeof(int));

	int subchunkSize = numInts / chunk_size + 1;

	std::vector<std::queue<int>> buffers(numChunks);
	for (int i = 0; i < buffers.size(); i++)
		replenishBuffer(buffers, chunkInfoArr, temp, subchunkSize, i);

	int counter = 0;
	std::vector<int> smallestValues(subchunkSize);
	std::priority_queue<bufferVal, std::vector<bufferVal>,
						std::greater<bufferVal>>
		minheap;

	for (int i = 0; i < buffers.size(); i++)
		minheap.push({buffers[i].front(), i});

	while (!minheap.empty()) {
		// 1. Get the smallest value across all the smallest values
		bufferVal minPair = minheap.top();
		minheap.pop();
		int minIndex = minPair.chunkIndex;

		// 2. Push it to an array of the smallest values, and remove it from its
		// respective queue
		smallestValues[counter++] = minPair.val;
		buffers[minIndex].pop();

		// 3. If that buffer is now empty, but has more integers to read, read
		// them
		if (buffers[minIndex].empty() and chunkInfoArr[minIndex].numsLeft > 0)
			replenishBuffer(buffers, chunkInfoArr, temp, subchunkSize,
							minIndex);

		// 4. If the buffer is not empty, push its minheap value (i.e first) to
		// the minheap
		if (!buffers[minIndex].empty())
			minheap.push({buffers[minIndex].front(), minIndex});

		// 5. If the array of minheap values is full, write to file and reset
		// the counter
		if (counter == subchunkSize) {
			out.write((char *)smallestValues.data(), counter * sizeof(int));
			counter = 0;
		}
	}

	if (counter > 0)
		out.write((char *)smallestValues.data(), counter * sizeof(int));
}

int main(int argc, char **argv) {
	std::ios::ios_base::sync_with_stdio(0);
	std::cout.tie(NULL);
	std::cin.tie(NULL);

	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << "<file_name>\n";
		return 1;
	}

	// TODO(?): eliminate temp file
	std::string fileName	 = argv[1];
	std::string tempFileName = "temp-" + fileName;
	std::string newFileName	 = "sorted-" + fileName;

	std::ofstream fileOut;
	std::fstream fileTemp;
	std::ifstream fileIn;

	std::vector<chunkInfo> chunkInfoArr;

	fileIn.open(fileName, std::ios::binary);
	fileTemp.open(tempFileName, std::ios::binary | std::ios::out);
	int numChunks = produceSortedChunks(fileIn, fileTemp, chunkInfoArr);
	fileIn.close();

	fileOut.open(newFileName, std::ios::binary);
	mergeSortedChunks(fileTemp, fileOut, chunkInfoArr, numChunks);
	fileTemp.close();
	fileOut.close();

	printFile(newFileName);

	return 0;
}
