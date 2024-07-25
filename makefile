SRC_FILES := main.c display.c commands.c log.c net.c ds/ll.c

# can't be bothered to write something fancy
a.out: $(SRC_FILES)
	gcc -g -Wall $^ -lncursesw -o $@
