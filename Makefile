CC		= gcc
CCOPTS  	= -g -Wall 
FLEXSRC 	= ltsNet.l 

wrapper: ltsNetCppWrapper.cc $(CPAROBJ)
	$(GPP) -c $(GPPOPTS) -I$(CUDD_INCLUDE) ltsNetCppWrapper.cc

$(CPAROBJ): $(BIFLSRC) $(CPARSRC)
	bison -d ltsNet.y
	flex ltsNet.l
	$(CC) -c $(CCOPTS) ltsNet.tab.c lex.yy.c $(CPARSRC) -lfl

clean:
	rm -rf *~ *.yy.c *.o ltsNet *.tab.*

parser: ltsNet.y ltsNet.l
	bison -d ltsNet.y
	flex ltsNet.l
	$(CC) -g ltsNet.tab.c lex.yy.c automata_interface.c -lfl


