CFLAGS =	-std=c11 -O0 -g -Wall -fmessage-length=0

OBJS =		fattree.o fat32.o work_with_file.o

LIBS =

TARGET =	fattree

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
