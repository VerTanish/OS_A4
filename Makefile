all:
	gcc  -m32 -no-pie -nostdlib -o fib fib.c
	gcc  -m32 -no-pie -nostdlib -o sum sum.c
	# gcc -m32 -o smartloader smartloader.c
	# gcc -m32 -o temp temp.c
	gcc -m32 -o wot wot.c


clean:
	-@rm -f fib loader