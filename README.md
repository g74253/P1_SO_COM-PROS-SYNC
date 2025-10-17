# P1_SO_COM-PROS-SYNC
Primer proyecto del curso Principio de Sistemas Operativos. 


---

## 1. Estructura del proyecto

```
.
├── Makefile
├── shared.h
├── ipc_utils.c
├── inicializador.c
├── emisor.c
├── receptor.c
├── finalizador.c
├── lorem.txt
├── output.txt
└── README.md
```

- `lorem.txt` Archivo de prueba
- `output.txt` Archivo de salida
- `inicializador.c` Inicializa la memoria compartida y todo lo necesario para el funcionamiento
- `emisor.c` Lee del archivo de entrada y escribe en ringbuffer
- `receptor.c` Lee del ringbuffer y lo escribe en el archivo de salida
- `finalizador.c` Finaliza ejecucion y libera memoria
- `shared.h` y `ipc_utils.c` Funciones auxiliares 

---

## 2. Test 

### Paso 1: Compilacion del programa
```bash
make
```
### Paso 2: Iniciar memoria compartida
```bash
./inicializador /shm 10 lorem.txt
```
El archivo lorem.txt tiene que estar en la misma direccion que el ejecutable.
### Paso 3: Iniciar finalizador
```bash
./finalizador /shm
```
### Paso 4: Archivo en tiempo real
```bash
watch -n 0.1 cat output.txt
```
### Paso 5: Iniciar Receptor
```bash
./receptor /shm auto 150 42
```
Se puede crear varios receptores.
### Paso 6: Iniciar finalizador
```bash
./emisor /shm auto 150 42
```
Se puede crear varios emisores.

---
