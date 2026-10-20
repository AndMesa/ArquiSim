/*
 * sistema.h -- Servicios del sistema (syscall), como los de MARS.
 *
 *   $v0 = 1   imprimir el entero de $a0
 *   $v0 = 4   imprimir la cadena (terminada en 0) que empieza en la dirección $a0
 *   $v0 = 5   leer un entero y dejarlo en $v0
 *   $v0 = 10  terminar el programa
 *   $v0 = 11  imprimir el carácter de $a0
 *   $v0 = 32  pausar $a0 milisegundos (en tiempo real)
 */
#ifndef SISTEMA_H
#define SISTEMA_H

#include "cpu.h"

Resultado llamar_sistema(CPU *cpu);

/* Agrega texto a la consola (se descarta lo más antiguo si se llena). */
void consola_agregar(CPU *cpu, const char *texto);

#endif
