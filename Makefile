CC		= gcc
CCOPTS  	= -g -Wall 
FLEXSRC 	= ltsNet.l 

all: ltsNet.y ltsNet.l tools.c automata_interface.c tree_topology.c
	bison -d ltsNet.y
	flex ltsNet.l
	$(CC) $(CCOPTS) tools.c ltsNet.tab.c lex.yy.c automata_interface.c tree_topology.c -lfl -o tree_reduce

clean:
	rm -rf *~ *.yy.c *.o ltsNet *.tab.*

dot:
	dot -Tpdf net.dot -o net.pdf
	dot -Tpdf sync.dot -o sync.pdf
	dot -Tpdf reduced.dot -o reduced.pdf
