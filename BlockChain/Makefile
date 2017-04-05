all: libblockchain.a

tar: libblockchain.a
	tar -cvf ex3.tar blockchain.cpp Block.h Block.cpp \
	Chain.h Chain.cpp libblockchain.a README Makefile

libblockchain.a: Chain.o Block.o blockchain.o
	ar rcs libblockchain.a Chain.o Block.o blockchain.o libblockchain.a

Chain.o: Chain.h Chain.cpp Block.h
	g++ -Wall -Wextra -Wvla -std=c++11 -c Chain.cpp

Block.o: Block.h Block.cpp
	g++ -Wall -Wextra -Wvla -std=c++11 -c Block.cpp

blockchain.o: blockchain.h blockchain.cpp Chain.h Block.h
	g++ -Wall -Wextra -Wvla -std=c++11 -c blockchain.cpp -L. -lhash -lcrypto -lpthread

clean:
	rm -f *.o libblockchain.a

.PHONT: clean tar all
