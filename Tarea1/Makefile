configure:
	sudo apt-get install -y libseccomp-dev

build_example:
	gcc -o Auxiliary/test_program Auxiliary/test_program.c

build_main:
	# sudo apt-get install libseccomp-dev
	gcc -o rastreador Source/*.c -lseccomp

clean:
	rm rastreador Auxiliary/test_program test_file.txt

run: build_example build_main
	./rastreador Auxiliary/test_program Arg1 -Arg2

run1: build_example build_main
	./rastreador -v Auxiliary/test_program Arg1 -Arg2

run2: build_example build_main
	./rastreador -V Auxiliary/test_program -Arg1 Arg2

run3: build_main
	./rastreador -v /usr/bin/tail test_file.txt

