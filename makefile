CC = g++
CFLAGS = -Wall -g

OBJS = Sapphire.o RDbf.o RTable.o RIndex.o RBtree.o RNode.o RPage.o RData.o RKey.o RField.o keyutil.o utility.o

sample:sample.o $(OBJS)
	g++ -Wall -g sample.o $(OBJS) -o sample
	
sapphire:${OBJS}
	rm -f libsapphire.a
	ar cq libsapphire.a $(OBJS)

.cpp.o:
	${CC} ${CFLAGS} -c $<
	

