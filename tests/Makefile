CC=gcc
CFLAGS=-Wall -DBUILD_LEGACY_SUPPORT

all: tests
default: tests

tests: test_interpreter

test_interpreter: test_interpreter.c interpreter.obj
	$(CC) $(CFLAGS) test_interpreter.c $(interpreter_obj) -o test_interpreter -lm

interpreter_incl :=\
	../src/interpreter/interpreter.h\
	../src/interpreter/parser.h\
	../src/interpreter/legacy.h\
	../src/interpreter/op.h

interpreter_src :=\
	../src/interpreter/interpreter.c\
	../src/interpreter/parser.c\
	../src/interpreter/legacy.c\
	../src/interpreter/op_datatype.c\
	../src/interpreter/op_encode.c\
	../src/interpreter/op_math.c\
	../src/interpreter/op_vitagrafix.c

interpreter_obj :=\
	interpreter.o\
	parser.o\
	legacy.o\
	op_datatype.o\
	op_encode.o\
	op_math.o\
	op_vitagrafix.o

interpreter: $(interpreter_src) $(interpreter_incl) main.c
	$(CC) $(CFLAGS) $(interpreter_src) main.c -o interpreter -lm

interpreter.obj: $(interpreter_src) $(interpreter_incl)
	$(CC) $(CFLAGS) -c $(interpreter_src) -lm

clean:
	rm test_interpreter interpreter *.o
