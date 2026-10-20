/*
 * terminal.h -- Funciones dependientes del sistema operativo (tamaño, pausas, Ctrl+C).
 */
#ifndef TERMINAL_H
#define TERMINAL_H

#include <signal.h>

extern volatile sig_atomic_t terminal_interrumpido;   /* Ctrl+C durante una animación */

void terminal_preparar(void);                /* UTF-8 y colores en Windows      */
int  terminal_salida_es_tty(void);           /* stdout es una terminal          */
int  terminal_interactiva(void);             /* stdin y stdout son terminales   */
void terminal_tamano(int *columnas, int *filas);
void terminal_pantalla_alterna(int activar); /* pantalla completa sin scroll    */
void terminal_dormir_ms(int ms);
void terminal_capturar_ctrl_c(void);
void terminal_animando(int activo);

/* Teclado no bloqueante: con teclas directas activas, terminal_tecla() devuelve la
 * tecla presionada (sin esperar Enter ni mostrarla) o -1 si no hay ninguna. */
void terminal_teclas_directas(int activar);
int  terminal_tecla(void);
long terminal_ms(void);                      /* reloj monotónico en milisegundos */

/* Lectura de una línea con edición básica e historial: flecha arriba/abajo recorren
 * los comandos anteriores, retroceso borra, Ctrl+U borra la línea. Devuelve 0 si
 * se cerró la entrada (Ctrl+D). */
#define MAX_HISTORIAL 50
#define MAX_COMANDO   256
typedef struct {
    char lineas[MAX_HISTORIAL][MAX_COMANDO];
    int  n;
} Historial;
int  terminal_leer_linea(char *buf, size_t tam, Historial *h);

/* Directorio de trabajo actual ("?" si no se puede obtener). */
void terminal_directorio(char *buf, size_t tam);

/* Lista (ordenados) los archivos .asm de un directorio. Devuelve cuántos encontró. */
#define MAX_NOMBRE_ARCHIVO 128
int  terminal_listar_asm(const char *directorio, char nombres[][MAX_NOMBRE_ARCHIVO], int max);

#endif
