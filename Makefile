Default:
rng:
	g++ generateRandomFile.cpp -o randomGenerator
sorter:
	g++ sort.cpp -o sort

gen:
	clear && ./randomGenerator random.bin 10
run:
	clear && ./sort random.bin

clean:
	rm -f randomGenerator sort *.bin