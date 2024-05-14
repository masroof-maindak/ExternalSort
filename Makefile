Default:
rng:
	g++ generateRandomFile.cpp -o randomGenerator
sorter:
	g++ sort.cpp -o sort

gen:
	clear && ./randomGenerator random.bin 4
run:
	clear && ./sort random.bin