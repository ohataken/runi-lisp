.c.o:
	$(CC) -Wall -Wextra -g -c $<

runi-lisp: runi_lisp.o main.o
	$(CC) -o runi-lisp $^

run: runi-lisp
	./$<
