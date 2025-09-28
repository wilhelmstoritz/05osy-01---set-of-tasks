TARGET1=generator

all: ${TARGET1} scitacka

${TARGET1}: ${TARGET1}.cpp
	g++ $^ -o $@

scitacka: scitacka.cpp
	g++ scitacka.cpp -o scitacka

clean:
	rm -f scitacka generator
