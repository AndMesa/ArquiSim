/*
 * tui.h -- Interfaz de pantalla completa: programa, IR y cambios a la izquierda;
 *          banco de registros y memoria a la derecha.
 */
#ifndef TUI_H
#define TUI_H

#include "cpu.h"
#include "isa.h"
#include "ensamblador.h"

/* Interfaz interactiva. Carga 'ruta' (o, si es NULL o tiene errores, muestra una
 * lista para elegir). Al salir deja en *p, *inicial y *final el programa, su
 * estado inicial y el estado final; devuelve las instrucciones ejecutadas, o -1
 * si se salió sin cargar ningún programa. */
int tui_ejecutar(const ISA *isa, const char *ruta, int max_pasos,
                 Programa *p, CPU *inicial, CPU *final);

/* Bordes con caracteres ASCII simples (+ - |) en lugar de dibujo de cajas. */
void tui_usar_ascii(int activar);

#endif
