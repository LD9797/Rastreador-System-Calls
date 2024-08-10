# Tarea Corta 1: RASTREADOR DE “System Calls”
**Instituto Tecnológico de Costa Rica**

**Escuela de Ingeniería en Computación**

**Maestría Académica en Ciencias de la Computación**

**Curso: Sistemas Operativos Avanzados**

**Profesor**: Dr. Francisco Torres

**Elaborado por**: 
  - Juan Jose Cordero 
  - Luis Diego Hidalgo
  - Esteban Villalobos

## Propósito

El propósito del programa reastreador es el de registrar todos los 
"system calls" que utilice algun otro programa que se indique, al ser 
este otro ejecutado, llamemos al rastrado "Prog".

El programa fue elaborado en el lenguage C, y probado en distribuciones
de Linux compatibles con Debian/Ubuntu/Mint. Existe un makefile con el
cual se pueden instalar dependencias y compilar el código fuente, además
se puede probar rastreador con algunas combinaciones de programas y 
comandos.

El codigo fuente cuenta con un programa de prueba a rastrear, 
el cual esta en `Auxiliary/test_program.c`. Este programa puede ser 
construido con el Makefile provisto.

## Uso de rastreador


La sintáxis de ejecución desde línea de comando es:

```rastreador [opciones rastreador] Prog [opciones de Prog] ```

Donde `Prog` es el programa a ejecutar y `[opciones de Prog]` son los
parámetros que el usuario le pasa a `Prog`, rastreador ejecuta `Prog` en el
background, pasándole sus argumentos, y despliega al final una tabla que 
sumariza el nombre de cada "System call" y la cantidad de veces que fue 
ejecutada por `Prog`.

### Opciones rastreador

-v despliega´a un mensaje cada vez que detecte un System Call de Prog. 
Despliega el nombre del system call llamado, así como los argumentos "crudos"
utilizados al llamar al syscall.

En el caso de los siguientes System Calls, decodificamos los argumentos "crudos"
para mostrar hileras de texto y otros valores pasados a la rutina de sistema:
   
   * openat
   * read
   * write


-V igual que -v , pero hace una pausa entre cada llamada a `rastreador`
después de cada "system call" hasta que el usuario presione cualquier 
tecla para continuar la ejecución de Prog .

## CONSTRUCCION Y EJECUCION DE rastreador

`make configure`: Se utiliza para instalar la única dependencia de rastreador,
la cual es libseccomp-dev.
	
`make build_example`: Compila y genera ejecutable para `test_program`. 
	
`make build_main`: Compila y genera el ejecutable de `rastreador`.

`make clean`: Borra los ejecutables `test_programa` y `rastreador`.
	
`make run`: Rastrea a test_program, sin opciones de `rastreador`, por ejemplo:

  ./rastreador Auxiliary/test_program Arg1 -Arg2

`make run1`: Rastrea a test_program, con la opción `-v` de `rastreador`, por ejemplo:

	./rastreador -v Auxiliary/test_program Arg1 -Arg2

`make run2`: Rastrea a test_program, con la opción `-V` de `rastreador`, por ejemplo:

	./rastreador -V Auxiliary/test_program -Arg1 Arg2

`make run3`: Rastrea el programa `tail` del S.O., utilizando la opción `-v` de rastreador:

	./rastreador -v /usr/bin/tail test_file.txt
 
