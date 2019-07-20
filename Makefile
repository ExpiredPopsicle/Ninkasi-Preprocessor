a : main.c ppmacro.c ppstate.c ppcommon.c
	$(CC) -g -Wall $^ -o $@
