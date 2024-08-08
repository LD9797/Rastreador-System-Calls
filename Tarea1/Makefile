configure:
	sudo apt-get install -y libseccomp-dev

build_example:
	gcc -o Auxiliary/test_program Auxiliary/test_program.c

build_main:
	# sudo apt-get install libseccomp-dev
	gcc -o rastreador Source/rastreador.c Source/file_utils.c Source/syscall_utils.c Source/terminal_utils.c -lseccomp

clean:
	rm rastreador Auxiliary/test_program

run: build_example build_main
	./rastreador Auxiliary/test_program

run1: build_example build_main
	./rastreador Auxiliary/test_program  -v

run2: build_example build_main
	./rastreador Auxiliary/test_program  -V

run3: build_main
	./rastreador -v /usr/bin/tail test_file.txt

