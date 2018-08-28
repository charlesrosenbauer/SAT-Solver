all:
	clang -O3 -lm -lSDL src/*.c -o bin/solver

debug:
	clang -O3 -g -lm -lSDL src/*.c -o bin/solver

slow:
	clang -O2 -g -lm -lSDL src/*.c -o bin/solver
