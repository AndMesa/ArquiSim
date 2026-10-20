/*
 * ciclo.h -- El ciclo de instrucción: Fetch -> Decode -> Execute.
 */
#ifndef CICLO_H
#define CICLO_H

#include "cpu.h"
#include "isa.h"
#include "ensamblador.h"

typedef enum { PASO_OK, PASO_FIN, PASO_EXCEPCION } EstadoPaso;

/* Ejecuta una instrucción. Devuelve PASO_FIN (sin ejecutar nada) si el PC
 * ya llegó al final del programa. */
EstadoPaso ejecutar_paso(const ISA *isa, const Programa *p, CPU *cpu, Instruccion *in);

/* Reinicia la máquina (registros y memoria en cero, $sp = 0x400, PC = 0) y carga
 * el programa. Devuelve 0 si hubo errores (quedan en p->errores). */
int cargar_programa(const ISA *isa, const char *ruta, Programa *p, CPU *cpu);

/* El programa terminó: el PC llegó al final de .text o se ejecutó syscall 10. */
int programa_terminado(const Programa *p, const CPU *cpu);

#endif
