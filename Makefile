CC=	gcc
LIBS=	-lm -lgd

all:	gdresize gdcreate gdcrop credits

gdresize:	gdresize.o utils.o
		$(CC) -o $@ utils.o gdresize.o $(LIBS)

gdcreate:	gdcreate.o utils.o
		$(CC) -o $@ utils.o gdcreate.o $(LIBS)

gdcrop:		gdcrop.o utils.o
		$(CC) -o $@ utils.o gdcrop.o $(LIBS)

credits:	credits.o utils.o
		$(CC) -o $@ utils.o credits.o $(LIBS)