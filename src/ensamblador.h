/*
 * ensamblador.h -- Traduce un programa .asm a binario y lo carga en la RAM.
 */
#ifndef ENSAMBLADOR_H
#define ENSAMBLADOR_H

#include "cpu.h"
#include "isa.h"

#define MAX_INSTR_PROGRAMA ((DATA_BASE - TEXT_BASE) / 4)
#define MAX_ETIQUETAS      128
#define MAX_NOMBRE         32
#define MAX_TEXTO          80

typedef struct {
    char     nombre[MAX_NOMBRE];
    uint32_t dir;
    int      linea;
} Etiqueta;

typedef struct {
    uint32_t dir;               /* dirección de la instrucción en RAM */
    int      linea;             /* línea en el archivo .asm           */
    char     texto[MAX_TEXTO];  /* texto original de la instrucción   */
} LineaFuente;

#define MAX_RUTA     256
#define MAX_ERRORES  2048

typedef struct {
    char        ruta[MAX_RUTA];
    uint32_t    text_fin;       /* primera dirección después del código */
    uint32_t    data_fin;       /* primera dirección después de los datos */
    int         num_instr;
    LineaFuente fuente[MAX_INSTR_PROGRAMA];
    int         num_etiquetas;
    Etiqueta    etiquetas[MAX_ETIQUETAS];
    char        errores[MAX_ERRORES];   /* mensajes de error del ensamblado */
    int         num_errores;
} Programa;

/* Ensambla y carga el programa en cpu->ram. Si hay errores devuelve 0 y los deja
 * en p->errores (una línea por error). */
int ensamblar(const char *ruta, const ISA *isa, CPU *cpu, Programa *p);

const LineaFuente *buscar_fuente(const Programa *p, uint32_t dir);
const char        *etiqueta_en(const Programa *p, uint32_t dir);   /* NULL si no hay */

#endif
