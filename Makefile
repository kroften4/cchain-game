all: server test_tsqueue

server: bin_path queue
	gcc -I. -ggdb ./build/ts_queue.o ./src/server.c -o ./bin/server

test_tsqueue: bin_path queue
	gcc -I. -ggdb ./build/ts_queue.o ./test/ts_queue.c -o ./bin/test_tsqueue

queue: build_path
	gcc -c ./src/ts_queue.c  -o ./build/ts_queue.o

bin_path:
	mkdir -p ./bin

build_path:
	mkdir -p ./build

clean:
	rm -r ./bin ./build
