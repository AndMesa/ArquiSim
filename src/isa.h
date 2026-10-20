/*
 * isa.h -- Carga del ISA (isa.txt) y decodificación de instrucciones.
 */
#ifndef ISA_H
#define ISA_H

#include "cpu.h"

#define MAX_INSTRUCCIONES_ISA 64

typedef struct {
    DefInstruccion inst[MAX_INSTRUCCIONES_ISA];
    int n;
} ISA;

extern const char *const NOMBRES_REG[NUM_REGISTROS];

int  cargar_isa(const char *ruta, ISA *isa);
const DefInstruccion *buscar_mnemonico(const ISA *isa, const char *mnemonico);

/* Una instrucción puede tener varias sintaxis (misma codificación). Busca la que
 * se escribe con 'escritos' operandos. */
const DefInstruccion *buscar_forma(const ISA *isa, const char *mnemonico, int escritos);
int  operandos_escritos(const DefInstruccion *d);   /* sin contar los implícitos (campo=$reg) */

/* Separa los 32 bits en campos y busca la instrucción en el ISA (la "unidad de control"). */
void decodificar(const ISA *isa, uint32_t binario, Instruccion *in);

/* "$t0", "$8", ... -> 8. Devuelve -1 si el nombre no es un registro. */
int  numero_registro(const char *nombre);

/* Interpreta "42", "-7", "0x2A", "0b101010". Devuelve 1 si es válido. */
int  leer_numero(const char *texto, long long *valor);

#endif
