OBJS = main.o bmp.o
CC = gcc
CFLAGS = -c

bmp2yuv : $(OBJS)
	$(CC) -o bmp2yuv $(OBJS) -lm -L/lib -L/usr/lib
main.o : main.c
	$(CC) $(CFLAGS) main.c
bmp.o : bmp.c
	$(CC) $(CFLAGS) bmp.c

clean:  
	rm *.o
	@echo "Clean done!"  



