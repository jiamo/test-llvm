CC = g++
CFLAGS = -o0 -g
LLVM_CONFIG = llvm-config
#LLVMFLAGS = $(shell $(LLVM_CONFIG) --cxxflags --ldflags --libs core jit native all)
LLVMFLAGS = $(shell $(LLVM_CONFIG) --cxxflags --ldflags --libs jit interpreter nativecodegen)
#LLVMVERSION = -lLLVM$(shell $(LLVM_CONFIG) --version)

all:
	${CC} ${CFLAGS} test_llvm.cpp -I/usr/lib/llvm-10/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -pthread -ldl -L/usr/lib/llvm-10/lib -lLLVM-10 -o test

clean:
	-rm *.o test