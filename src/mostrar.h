/*
 * mostrar.h -- Presentación del estado de la máquina en la terminal.
 */
#ifndef MOSTRAR_H
#define MOSTRAR_H

#include <stddef.h>
#include "cpu.h"
#include "isa.h"
#include "ensamblador.h"

void configurar_color(int activar);

void mostrar_titulo(const char *titulo);
void mostrar_arquitectura(const Programa *p);
void mostrar_programa(const CPU *cpu, const Programa *p);
void mostrar_paso(int paso, const CPU *antes, const CPU *despues,
                  const Instruccion *in, const Programa *p, int con_tabla);
void mostrar_registros(const CPU *cpu, const CPU *referencia);
void mostrar_memoria(const CPU *cpu, const Programa *p, uint32_t dir, int palabras);
void mostrar_final(const CPU *cpu, const CPU *inicial, const Programa *p, int pasos);
void mostrar_error(const char *formato, ...);

/* Separa una instrucción en campos: bits, nombres y valores (3 líneas).
 * Devuelve el número de líneas escritas. */
#define MAX_CAMPOS 200
int  formatear_campos(const Instruccion *in, uint32_t pc, char lineas[3][MAX_CAMPOS]);

/* "Z 0  N 1  C 0  V 0": las banderas que cambiaron respecto a ref se resaltan. */
void formatear_banderas(const CPU *cpu, const CPU *ref, char *buf, size_t tam);

/* "R 5  I 6  J 1 | lect. 2  escr. 1 | saltos 3" */
void formatear_estadisticas(const CPU *cpu, char *buf, size_t tam);

/* Pantalla mapeada en memoria */
#define MAX_FILA_PANTALLA 512
int  pantalla_usada(const CPU *cpu);
void formatear_fila_pantalla(const CPU *cpu, int y, int px, char *buf, size_t tam);
int  rgb_usada(const CPU *cpu);
void formatear_fila_rgb(const CPU *cpu, int y, int px, char *buf, size_t tam);
/* Dos filas de píxeles (y, y+1) en una línea de texto con el medio bloque '▀'. */
void formatear_dos_filas_rgb(const CPU *cpu, int y, int px, char *buf, size_t tam);

/* Secuencias de color (cadena vacía si el color está desactivado) */
const char *color_reset(void);
const char *color_rojo(void);
const char *color_verde(void);
const char *color_tenue(void);
const char *color_negrita(void);
/* Temas de color */
const char *nombre_tema(void);
void siguiente_tema(void);
int  elegir_tema(const char *nombre);
const char *lista_temas(void);

const char *color_cambio(void);     /* valores que cambiaron, instrucción actual */
const char *color_titulo(void);     /* títulos y última instrucción ejecutada    */

#endif
