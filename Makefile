CC		= gcc
CCOPTS  	= -g -Wall -Wno-implicit-function-declaration
FLEXSRC 	= ltsNet.l

all: ltsNet.y ltsNet.l tools.c automata_interface.c tree_topology.c main.c
	bison -d ltsNet.y
	flex ltsNet.l
	$(CC) $(CCOPTS) tools.c ltsNet.tab.c lex.yy.c automata_interface.c tree_topology.c main.c -o tree_reduce

clean:
	rm -rf *~ *.yy.c *.o ltsNet *.tab.* *.dot *.pdf

dot:
	dot -Tpdf net.dot -o net.pdf
	dot -Tpdf sync.dot -o sync.pdf
	dot -Tpdf reduced.dot -o reduced.pdf
