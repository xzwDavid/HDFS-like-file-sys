SRC=$(shell find ./ -name "*.c") # 找到文件下面所有的.c
SHELLOBJS=$(SRC:%.c=%.o)
CC=gcc -std=c99
Target=shell

$(Target): $(SHELLOBJS)
	$(CC) -g -o $(Target) $(SHELLOBJS)
#	$(CC) -g -o shell $(SHELLOBJS) -Wall
%.o:%.c 
	$(CC) -c $< -o $@

clean:
	rm $(SHELLOBJS)
	rm $(Target)
