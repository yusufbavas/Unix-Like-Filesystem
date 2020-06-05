all:
	gcc makeFileSystem.c -lm -std=gnu11 -Wall -pedantic-errors -o makeFileSystem
	gcc fileSystemOper.c directoryOperations.c fileOperations.c -lm -std=gnu11 -Wall -pedantic-errors -o fileSystemOper
