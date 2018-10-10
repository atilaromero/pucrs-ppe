all: m1 m2 m3
m1: matrix_multi1.cpp
	g++ -std=c++1y -O3 -ltbb -lpthread matrix_multi1.cpp -o m1

m2: matrix_multi2.cpp
	g++ -std=c++1y -O3 -ltbb -lpthread matrix_multi2.cpp -o m2

m3: matrix_multi3.cpp
	g++ -std=c++1y -O3 -ltbb -lpthread matrix_multi3.cpp -o m3
