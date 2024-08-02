build_example:
	gcc -o test_program test_program.c

build_main:
	# sudo apt-get install libseccomp-dev
	gcc -o rastradeador rastradeador.c -lseccomp

clean:
	rm rastradeador test_program

run: build_example build_main
	./rastradeador test_program