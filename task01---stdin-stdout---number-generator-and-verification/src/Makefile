TARGET_GEN=gennum
TARGET_VER=verbank

all: ${TARGET_GEN} ${TARGET_VER}

${TARGET_GEN}: ${TARGET_GEN}.cpp
	g++ $^ -o $@

${TARGET_VER}: ${TARGET_VER}.cpp
	g++ $^ -o $@

clean:
	rm -f ${TARGET_GEN} ${TARGET_VER}
