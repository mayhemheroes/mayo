TEMPLATE = app
SOURCES = fuzz_consoleToPrintable.cpp
LIBS += -L/repo/mayo-build -lmayo -fsanitize=fuzzer,address
TARGET = fuzz
QMAKE_CXXFLAGS += -fsanitize=fuzzer,address
QMAKE_CXX=clang++
QMAKE_LINK=clang++
INCLUDEPATH += /repo/src/app
CONFIG += c++17
