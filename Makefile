all: build

run: build
	./bin/shared_benchmark
	./bin/pipe_benchmark

build:
	mkdir -p ./bin
	g++ -O2 shared_benchmark.cpp -o ./bin/shared_benchmark
	g++ -O2 pipe_benchmark.cpp -o ./bin/pipe_benchmark

clean:
	rm -rf ./bin
