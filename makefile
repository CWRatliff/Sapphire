CC = g++
CFLAGS = -Wall -g

OBJS = Sapphire.o RDbf.o RTable.o RIndex.o RBtree.o RNode.o RPage.o RData.o RKey.o RField.o keyutil.o utility.o

sapphire:${OBJS}

.cpp.o:
	${CC} ${CFLAGS} -c $<
