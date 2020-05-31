CC		= gcc
CCOPTS  	= -g -Wall 
FLEXSRC 	= ltsNet.l 

all: ltsNet.y ltsNet.l tools.c automata_interface.c
	bison -d ltsNet.y
	flex ltsNet.l
	$(CC) $(CCOPTS) tools.c ltsNet.tab.c lex.yy.c automata_interface.c -lfl

clean:
	rm -rf *~ *.yy.c *.o ltsNet *.tab.*



