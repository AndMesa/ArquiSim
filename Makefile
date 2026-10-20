# ArquiSim
#   make         compila ./sim
#   make test    ejecuta todos los programas de prueba
#   make clean   borra el ejecutable

CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -O2
SRC     = src/main.c src/ciclo.c src/cpu.c src/isa.c src/ensamblador.c src/mostrar.c \
          src/terminal.c src/tui.c src/sistema.c src/ejecutar.c
HDR     = $(wildcard src/*.h)
SIM    ?= ./sim

sim: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test: sim
	@pasan=0; total=0; \
	for f in programas/*.asm; do \
	  total=$$((total + 1)); \
	  if $(SIM) -t $$f; then pasan=$$((pasan + 1)); fi; \
	done; \
	echo ""; echo "Resultado: $$pasan de $$total programas pasan"

clean:
	rm -f sim sim.exe

.PHONY: test clean
