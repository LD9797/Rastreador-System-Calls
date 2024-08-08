configure:
	sudo apt-get install -y libseccomp-dev

build_example:
	gcc -o test_program test_program.c

build_main:
	# sudo apt-get install libseccomp-dev
	gcc -o rastreador rastreador.c -lseccomp

alt_build_main:
	gcc -I Headers -o rastreador Source/rastreador.c Source/util.c -lseccomp

clean:
	rm rastreador test_program

run: build_example build_main
	./rastreador test_program

run1: build_example build_main
	./rastreador test_program  -v

run2: build_example build_main
	./rastreador test_program  -V

run3: build_main
	./rastreador /usr/bin/tail test_file.txt

