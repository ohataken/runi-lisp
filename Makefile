.c.o:
	$(CC) -Wall -Wextra -g -c $<

runi-lisp: main.o
	$(CC) -o runi-lisp $^

run: runi-lisp
	./$<
