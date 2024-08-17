# RASTREADOR DE “System Calls”

**Elaborado por**: 
  - Juan Jose Cordero 
  - Luis Diego Hidalgo
  - Esteban Villalobos

## Propósito

El propósito del programa `rastreador` es el de registrar todos los 
"System Calls" que utilice algun otro programa que se indique, al ser 
este otro ejecutado, llamemos al rastrado `Prog`.

El programa fue elaborado en el lenguage C, y probado en distribuciones
de Linux compatibles con Debian/Ubuntu/Mint. Existe un makefile con el
cual se pueden instalar dependencias y compilar el código fuente, además
se puede probar rastreador con algunas combinaciones de programas y 
comandos.

El codigo fuente cuenta con un programa de prueba a rastrear, 
el cual esta en `Auxiliary/test_program.c`. Este programa puede ser 
construido con el Makefile provisto. El programa puede recibir "n" argumentos, los cuales serán impresos en la salida estandar, posteriormente el programa abre un archivo de texto y escribe una línea en el mismo.

## Uso de `rastreador`


La sintáxis de ejecución desde línea de comando es:

```rastreador [opciones rastreador] Prog [opciones de Prog] ```

Donde `Prog` es el programa a ejecutar y `[opciones de Prog]` son los
parámetros que el usuario le pasa a `Prog`, rastreador ejecuta `Prog` en el
background, pasándole sus argumentos, y despliega al final una tabla que 
sumariza el nombre de cada "System call" y la cantidad de veces que fue 
ejecutada por `Prog`.

### Opciones de `rastreador`

`-v` despliegará un mensaje cada vez que detecte un System Call de Prog. 
Se imprime el nombre del System Call llamado, así como los argumentos "crudos"
utilizados al llamar el syscall.

En el caso de los siguientes System Calls, decodificamos los argumentos "crudos"
para mostrar hileras de texto y otros valores pasados a la rutina de sistema:
   
   * openat
   * read
   * write

`-V` hace una pausa entre cada llamada a `rastreador`
después de cada "System Call" hasta que el usuario presione cualquier 
tecla para continuar la ejecución de Prog.

Las opciones `-v` y `-V` son excluyentes.

## Construcción y Ejecución de `rastreador`

`make configure`: Se utiliza para instalar la única dependencia de `rastreador`,
la cual es libseccomp-dev.
	
`make build_example`: Compila y genera ejecutable para `test_program`. 
	
`make build_main`: Compila y genera el ejecutable de `rastreador`.

`make clean`: Borra los ejecutables `test_program` y `rastreador`.
	
`make run`: Rastrea a `test_program`, sin opciones de `rastreador`, muestra una tabla con un recuento de los System Call realizados por el programa objetivo, por ejemplo:

  `./rastreador Auxiliary/test_program Arg1 Arg2`

`make run1`: Rastrea a `test_program`, con la opción `-v` de `rastreador`, muestra cada System Call realizado por el programa objetivo, incluyendo la tabla resumen, por ejemplo:

	./rastreador -v Auxiliary/test_program Arg1 Arg2

`make run2`: Rastrea a `test_program`, con la opción `-V` de `rastreador`, muestra cada System Call realizado por el programa objetivo y hace una pausa, solicitando que el usuario presione cualquier tecla para continuar, incluyendo la tabla resumen, por ejemplo:

	./rastreador -V Auxiliary/test_program Arg1 Arg2

`make run3`: Rastrea el programa `tail` del S.O. utilizando la opción `-v` de rastreador, demostrando que el rastreador permite el uso de parametrós en el programa objetivo:

	./rastreador -v /usr/bin/tail test_file.txt

## Lo que NO funciona

- No se puede ejecutar `rastreador` con ambas opciones `-vV`, ni `Vv`, pues son excluyentes. Si
se intenta se obtiene una respuesta como esta:

```
Unrecognized option: -vV
Usage: ./rastreador [-v] [-V] <program to trace> [program args...]
```
