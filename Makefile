a : main.c ppmacro.c ppstate.c ppcommon.c ppstring.c pptoken.c ppdirect.c
	$(CC) -g -Wall $^ -o $@
