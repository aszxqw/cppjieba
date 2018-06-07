g++ -O3 -c -fPIC Jieba.cpp -o Jieba.o
g++ -O3 -shared -Wl,-soname,libJieba.so -o libJieba.so  Jieba.o

