/*
 * ensamblador.c -- Ensamblador de dos pasadas:
 *   1a pasada: calcula la dirección de cada etiqueta.
 *   2a pasada: codifica cada instrucción según isa.txt y la escribe en la RAM.
 *
 * Sintaxis soportada:
 *   # comentario
 *   .text / .data                 cambia de segmento
 *   ETIQUETA:                     define una etiqueta (puede ir antes de una instrucción)
 *   .word v1, v2, ...             palabras de 32 bits (números o etiquetas)
 *   .space n                      reserva n bytes en cero (redondeado a palabras)
 *   mnemonico op1, op2, ...       según los operandos definidos en isa.txt
 * Valores: 42, -7, 0x2A, 0b101010, 'a' o el nombre de una etiqueta (su dirección).
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ensamblador.h"

typedef enum { SEC_TEXT, SEC_DATA } Seccion;

typedef struct {
    const ISA *isa;
    CPU       *cpu;
    Programa  *p;
    int        pasada;
    Seccion    sec;
    uint32_t   pc_text;
    uint32_t   pc_data;
} Ctx;

static void agregar_error(Programa *p, const char *formato, va_list ap)
{
    size_t usado = strlen(p->errores);

    p->num_errores++;
    if (usado + 1 >= sizeof p->errores)
        return;
    vsnprintf(p->errores + usado, sizeof p->errores - usado, formato, ap);
    usado = strlen(p->errores);
    if (usado + 1 < sizeof p->errores)
        strcat(p->errores, "\n");
}

static void error_general(Programa *p, const char *formato, ...)
{
    va_list ap;

    va_start(ap, formato);
    agregar_error(p, formato, ap);
    va_end(ap);
}

/* Los errores se reportan en la 2a pasada, cuando todas las etiquetas ya se conocen. */
static void error_linea(Ctx *c, int linea, const char *formato, ...)
{
    va_list ap;
    char prefijo[MAX_RUTA + 64], completo[MAX_RUTA + 512];

    if (c->pasada != 2)
        return;
    snprintf(prefijo, sizeof prefijo, "%s:%d: error: ", c->p->ruta, linea);
    va_start(ap, formato);
    vsnprintf(completo, sizeof completo, formato, ap);
    va_end(ap);
    error_general(c->p, "%s%s", prefijo, completo);
}

static char *recortar(char *s)
{
    char *fin;

    while (isspace((unsigned char)*s))
        s++;
    fin = s + strlen(s);
    while (fin > s && isspace((unsigned char)fin[-1]))
        *--fin = '\0';
    return s;
}

static int es_identificador(const char *s)
{
    if (!isalpha((unsigned char)*s) && *s != '_')
        return 0;
    for (s++; *s; s++)
        if (!isalnum((unsigned char)*s) && *s != '_')
            return 0;
    return 1;
}

/* Divide s por comas (en el mismo arreglo). Devuelve cuántas partes hay. */
static int dividir(char *s, char *partes[], int max)
{
    int n = 0;
    char *coma;

    s = recortar(s);
    if (*s == '\0')
        return 0;
    for (;;) {
        coma = strchr(s, ',');
        if (coma)
            *coma = '\0';
        if (n < max)
            partes[n] = recortar(s);
        n++;
        if (!coma)
            break;
        s = coma + 1;
    }
    return n;
}

static const Etiqueta *buscar_etiqueta(const Programa *p, const char *nombre)
{
    int i;

    for (i = 0; i < p->num_etiquetas; i++)
        if (strcmp(p->etiquetas[i].nombre, nombre) == 0)
            return &p->etiquetas[i];
    return NULL;
}

const char *etiqueta_en(const Programa *p, uint32_t dir)
{
    int i;

    for (i = 0; i < p->num_etiquetas; i++)
        if (p->etiquetas[i].dir == dir)
            return p->etiquetas[i].nombre;
    return NULL;
}

const LineaFuente *buscar_fuente(const Programa *p, uint32_t dir)
{
    int i;

    for (i = 0; i < p->num_instr; i++)
        if (p->fuente[i].dir == dir)
            return &p->fuente[i];
    return NULL;
}

/* Número, carácter ('a') o etiqueta. */
static int valor(Ctx *c, int linea, const char *s, long long *v)
{
    const Etiqueta *e;

    if (*s == '\0') {
        error_linea(c, linea, "falta un valor");
        return 0;
    }
    if (s[0] == '\'') {
        if (strlen(s) == 3 && s[2] == '\'') {
            *v = (unsigned char)s[1];
            return 1;
        }
        error_linea(c, linea, "carácter inválido %s (use 'a')", s);
        return 0;
    }
    if (es_identificador(s)) {
        e = buscar_etiqueta(c->p, s);
        if (!e) {
            error_linea(c, linea, "la etiqueta '%s' no está definida", s);
            return 0;
        }
        *v = e->dir;
        return 1;
    }
    if (!leer_numero(s, v)) {
        error_linea(c, linea, "valor inválido '%s'", s);
        return 0;
    }
    return 1;
}

static int registro(Ctx *c, int linea, const char *s, uint32_t *r)
{
    int n = numero_registro(s);

    if (n < 0) {
        error_linea(c, linea, "registro inválido '%s' (use $t0, $s1, $8, ...)", s);
        return 0;
    }
    *r = (uint32_t)n;
    return 1;
}

static int inmediato(Ctx *c, int linea, const char *s, long long min, long long max, uint32_t *imm)
{
    long long v;

    if (!valor(c, linea, s, &v))
        return 0;
    if (v < min || v > max) {
        error_linea(c, linea, "el valor %lld no cabe en el campo de 16 bits (%lld a %lld)", v, min, max);
        return 0;
    }
    *imm = (uint32_t)v & 0xFFFF;
    return 1;
}

static void definir_etiqueta(Ctx *c, int linea, const char *nombre)
{
    Programa *p = c->p;
    const Etiqueta *e = buscar_etiqueta(p, nombre);
    Etiqueta *nueva;

    if (c->pasada == 1) {
        if (e || p->num_etiquetas >= MAX_ETIQUETAS || strlen(nombre) >= MAX_NOMBRE)
            return;     /* se reporta en la 2a pasada */
        nueva = &p->etiquetas[p->num_etiquetas++];
        strcpy(nueva->nombre, nombre);
        nueva->dir   = (c->sec == SEC_TEXT) ? c->pc_text : c->pc_data;
        nueva->linea = linea;
        return;
    }
    if (strlen(nombre) >= MAX_NOMBRE)
        error_linea(c, linea, "nombre de etiqueta demasiado largo");
    else if (!e)
        error_linea(c, linea, "demasiadas etiquetas (máximo %d)", MAX_ETIQUETAS);
    else if (e->linea != linea)
        error_linea(c, linea, "la etiqueta '%s' ya fue definida en la línea %d", nombre, e->linea);
}

/* Alinea el contador de datos a 'bytes' (potencia de 2). Las etiquetas que
 * apuntaban a la posición sin alinear ("A:" en una línea y ".word" en la
 * siguiente) se mueven con él. */
static void alinear_datos(Ctx *c, uint32_t bytes)
{
    uint32_t viejo = c->pc_data, nuevo = (viejo + bytes - 1) & ~(bytes - 1);
    int i;

    if (nuevo == viejo)
        return;
    if (c->pasada == 1)
        for (i = c->p->num_etiquetas - 1; i >= 0 && c->p->etiquetas[i].dir == viejo; i--)
            c->p->etiquetas[i].dir = nuevo;
    c->pc_data = nuevo;
}

/* "texto con \n" -> bytes. Devuelve la longitud o -1 si el formato es inválido. */
static int leer_cadena(const char *s, char *out, size_t tam)
{
    size_t n = 0;

    if (*s++ != '"')
        return -1;
    for (; *s && *s != '"'; s++) {
        char ch = *s;
        if (ch == '\\') {
            s++;
            ch = *s == 'n' ? '\n' : *s == 't' ? '\t' : *s == '0' ? '\0' : *s;
            if (!*s)
                return -1;
        }
        if (n + 1 >= tam)
            return -1;
        out[n++] = ch;
    }
    if (*s != '"' || recortar((char *)s + 1)[0] != '\0')
        return -1;
    out[n] = '\0';
    return (int)n;
}

static void poner_byte_datos(Ctx *c, int linea, uint8_t v)
{
    if (c->pasada == 2) {
        if (c->pc_data >= RAM_BYTES)
            error_linea(c, linea, "los datos no caben en la RAM");
        else
            c->cpu->ram[c->pc_data] = v;
    }
    c->pc_data++;
}

static void directiva(Ctx *c, int linea, char *s)
{
    char *nombre = s, *resto = s, *partes[64], cadena[512];
    int n, i;
    long long v;

    while (*resto && !isspace((unsigned char)*resto))
        resto++;
    if (*resto)
        *resto++ = '\0';
    resto = recortar(resto);

    if (strcmp(nombre, ".text") == 0) {
        c->sec = SEC_TEXT;
        return;
    }
    if (strcmp(nombre, ".data") == 0) {
        c->sec = SEC_DATA;
        return;
    }
    if (strcmp(nombre, ".globl") == 0 || strcmp(nombre, ".global") == 0)
        return;                                     /* compatibilidad con MARS */
    if (c->sec != SEC_DATA) {
        error_linea(c, linea, "%s solo se permite en la sección .data", nombre);
        return;
    }

    if (strcmp(nombre, ".word") == 0 || strcmp(nombre, ".byte") == 0) {
        int palabra = nombre[1] == 'w';
        if (palabra)
            alinear_datos(c, 4);
        n = dividir(resto, partes, 64);
        if (n == 0)
            error_linea(c, linea, "%s necesita al menos un valor", nombre);
        if (n > 64) {
            error_linea(c, linea, "demasiados valores en una sola línea (máximo 64)");
            n = 64;
        }
        for (i = 0; i < n; i++) {
            v = 0;
            if (c->pasada == 2 && !valor(c, linea, partes[i], &v))
                continue;
            if (!palabra) {
                if (c->pasada == 2 && (v < -128 || v > 255))
                    error_linea(c, linea, "el valor %lld no cabe en un byte", v);
                poner_byte_datos(c, linea, (uint8_t)v);
            } else {
                if (c->pasada == 2) {
                    if (c->pc_data + 4 > RAM_BYTES)
                        error_linea(c, linea, "los datos no caben en la RAM");
                    else
                        poner_palabra(c->cpu, c->pc_data, (uint32_t)v);
                }
                c->pc_data += 4;
            }
        }
    } else if (strcmp(nombre, ".asciiz") == 0 || strcmp(nombre, ".ascii") == 0) {
        n = leer_cadena(resto, cadena, sizeof cadena);
        if (n < 0) {
            error_linea(c, linea, "%s necesita una cadena entre comillas, p. ej. \"Hola\\n\"", nombre);
            return;
        }
        for (i = 0; i < n; i++)
            poner_byte_datos(c, linea, (uint8_t)cadena[i]);
        if (nombre[6] == 'z')
            poner_byte_datos(c, linea, 0);          /* .asciiz: termina en 0 */
    } else if (strcmp(nombre, ".space") == 0) {
        if (!leer_numero(resto, &v) || v <= 0 || v > RAM_BYTES) {
            error_linea(c, linea, ".space necesita un número de bytes positivo");
            return;
        }
        c->pc_data += (uint32_t)v;
        if (c->pc_data > RAM_BYTES)
            error_linea(c, linea, "los datos no caben en la RAM");
    } else if (strcmp(nombre, ".align") == 0) {
        if (!leer_numero(resto, &v) || v < 0 || v > 3) {
            error_linea(c, linea, ".align necesita 0, 1, 2 o 3 (alinear a 1, 2, 4 u 8 bytes)");
            return;
        }
        alinear_datos(c, 1u << v);
    } else {
        error_linea(c, linea, "directiva desconocida '%s' (use .text, .data, .word, .byte, "
                    ".asciiz, .ascii, .space, .align)", nombre);
    }
}

static uint32_t codificar(const DefInstruccion *d, uint32_t rs, uint32_t rt, uint32_t rd,
                          uint32_t shamt, uint32_t imm, uint32_t dir)
{
    uint32_t b = d->opcode << 26;

    switch (d->formato) {
    case FMT_R: b |= rs << 21 | rt << 16 | rd << 11 | shamt << 6 | (uint32_t)d->funct; break;
    case FMT_I: b |= rs << 21 | rt << 16 | (imm & 0xFFFF); break;
    case FMT_J: b |= dir & 0x03FFFFFF; break;
    }
    return b;
}

static void agregar_fuente(Ctx *c, uint32_t dir, int linea, const char *formato, ...)
{
    LineaFuente *lf = &c->p->fuente[c->p->num_instr++];
    va_list ap;

    lf->dir = dir;
    lf->linea = linea;
    va_start(ap, formato);
    vsnprintf(lf->texto, sizeof lf->texto, formato, ap);
    va_end(ap);
}

/* Saltos de comparación blt/bgt/ble/bge: el ensamblador los expande con $at.
 * Devuelve cuántas instrucciones ocupa (2, o 3 si el segundo operando es un
 * número) o 0 si la línea no es uno de ellos. */
#define REG_AT 1
static int es_rama_comparacion(const char *s, char *mn, char *op1, char *op2, char *etiqueta, size_t tam)
{
    char copia[MAX_TEXTO], *partes[4], *ops;
    long long v;
    int n, i;

    for (i = 0; i < 3 && s[i]; i++)
        mn[i] = (char)tolower((unsigned char)s[i]);
    mn[i] = '\0';
    if (!isspace((unsigned char)s[3]) ||
        (strcmp(mn, "blt") && strcmp(mn, "bgt") && strcmp(mn, "ble") && strcmp(mn, "bge")))
        return 0;
    snprintf(copia, sizeof copia, "%s", s + 4);
    ops = recortar(copia);
    n = dividir(ops, partes, 4);
    if (n != 3)
        return 0;
    snprintf(op1, tam, "%s", partes[0]);
    snprintf(op2, tam, "%s", partes[1]);
    snprintf(etiqueta, MAX_NOMBRE, "%s", partes[2]);
    return (leer_numero(op2, &v) || op2[0] == '\'') ? 3 : 2;
}

static void rama_comparacion(Ctx *c, int linea, uint32_t pc, int tam, const char *mn,
                             const char *op1, const char *op2, const char *etiqueta)
{
    const DefInstruccion *slt = buscar_forma(c->isa, "slt", 3);
    const DefInstruccion *addi = buscar_forma(c->isa, "addi", 3);
    int menor = strcmp(mn, "blt") == 0 || strcmp(mn, "bge") == 0;   /* rs < X  o  X < rs */
    int salta_si_1 = strcmp(mn, "blt") == 0 || strcmp(mn, "bgt") == 0;
    const DefInstruccion *br = buscar_forma(c->isa, salta_si_1 ? "bne" : "beq", 3);
    const Etiqueta *e = buscar_etiqueta(c->p, etiqueta);
    uint32_t rs, x, imm = 0, dir = pc;
    long long v;

    if (!slt || !br || (tam == 3 && !addi)) {
        error_linea(c, linea, "%s se expande a slt + %s%s, que no están en el ISA", mn,
                    salta_si_1 ? "bne" : "beq", tam == 3 ? " (y addi)" : "");
        return;
    }
    if (!registro(c, linea, op1, &rs))
        return;
    if (!e) {
        error_linea(c, linea, "la etiqueta '%s' no está definida", etiqueta);
        return;
    }
    if (tam == 3) {                     /* el segundo operando es un número: cargarlo en $at */
        if (!valor(c, linea, op2, &v))
            return;
        if (v < -32768 || v > 32767) {
            error_linea(c, linea, "el valor %lld no cabe en 16 bits", v);
            return;
        }
        escribir_palabra(c->cpu, dir, codificar(addi, 0, REG_AT, 0, 0, (uint32_t)v, 0));
        agregar_fuente(c, dir, linea, "addi $at, $zero, %lld   (%s %s, %s, %s)", v, mn, op1, op2, etiqueta);
        dir += 4;
        x = REG_AT;
    } else if (!registro(c, linea, op2, &x)) {
        return;
    }
    /* slt $at, rs, X  (blt/bge)   o   slt $at, X, rs  (bgt/ble) */
    escribir_palabra(c->cpu, dir, codificar(slt, menor ? rs : x, menor ? x : rs, REG_AT, 0, 0, 0));
    if (tam == 3)
        agregar_fuente(c, dir, linea, "slt  $at, %s, %s", menor ? op1 : "$at", menor ? "$at" : op1);
    else
        agregar_fuente(c, dir, linea, "slt  $at, %s, %s   (%s %s, %s, %s)", menor ? op1 : op2,
                       menor ? op2 : op1, mn, op1, op2, etiqueta);
    dir += 4;
    imm = (uint32_t)(((long long)e->dir - (long long)(dir + 4)) / 4);
    escribir_palabra(c->cpu, dir, codificar(br, REG_AT, 0, 0, 0, imm, 0));
    agregar_fuente(c, dir, linea, "%s  $at, $zero, %s", salta_si_1 ? "bne" : "beq", etiqueta);
}

/* Pseudoinstrucción "li rt, valor" con un valor que no cabe en 16 bits con signo:
 * se expande a dos instrucciones, lui (16 bits altos) + ori (16 bits bajos). */
static int es_li_largo(const char *s, char *reg, size_t tam, long long *v)
{
    char copia[MAX_TEXTO], *partes[4], *ops;
    int n;

    if (tolower((unsigned char)s[0]) != 'l' || tolower((unsigned char)s[1]) != 'i' ||
        !isspace((unsigned char)s[2]))
        return 0;
    snprintf(copia, sizeof copia, "%s", s + 3);
    ops = recortar(copia);
    n = dividir(ops, partes, 4);
    if (n != 2 || !leer_numero(partes[1], v) || (*v >= -32768 && *v <= 32767))
        return 0;
    snprintf(reg, tam, "%s", partes[0]);
    return 1;
}

static void li_largo(Ctx *c, int linea, uint32_t pc, const char *texto_reg, long long v)
{
    const DefInstruccion *lui = buscar_forma(c->isa, "lui", 2);
    const DefInstruccion *ori = buscar_forma(c->isa, "ori", 3);
    uint32_t rt, hi = ((uint32_t)v >> 16) & 0xFFFF, lo = (uint32_t)v & 0xFFFF;
    LineaFuente *lf;

    if (!lui || !ori) {
        error_linea(c, linea, "li con un valor de 32 bits se expande a lui + ori, que no están en el ISA");
        return;
    }
    if (!registro(c, linea, texto_reg, &rt))
        return;
    escribir_palabra(c->cpu, pc, lui->opcode << 26 | rt << 16 | hi);
    escribir_palabra(c->cpu, pc + 4, ori->opcode << 26 | rt << 21 | rt << 16 | lo);

    lf = &c->p->fuente[c->p->num_instr++];
    lf->dir = pc;
    lf->linea = linea;
    snprintf(lf->texto, sizeof lf->texto, "lui  %s, 0x%04X        (li %s, 0x%08X)", texto_reg, hi,
             texto_reg, (uint32_t)v);
    lf = &c->p->fuente[c->p->num_instr++];
    lf->dir = pc + 4;
    lf->linea = linea;
    snprintf(lf->texto, sizeof lf->texto, "ori  %s, %s, 0x%04X", texto_reg, texto_reg, lo);
}

static void instruccion(Ctx *c, int linea, char *s)
{
    const DefInstruccion *d;
    char texto[MAX_TEXTO], patron[MAX_PATRON], formas[160];
    char *mn, *ops, *p, *partes[4], *tok[6], *par, *cierre, *desp, *extra;
    int n, m, i, j, ok = 1;
    uint32_t reg, tam = 4;
    char reg_li[16], rama[4], op1[MAX_TEXTO], op2[MAX_TEXTO], et[MAX_NOMBRE];
    long long valor_li;
    int n_rama;
    long long v;
    uint32_t pc = c->pc_text, rs = 0, rt = 0, rd = 0, shamt = 0, imm = 0, dir = 0, b;
    const Etiqueta *e;
    LineaFuente *lf;

    if (c->sec != SEC_TEXT) {
        error_linea(c, linea, "las instrucciones deben ir en la sección .text");
        return;
    }
    n_rama = es_rama_comparacion(s, rama, op1, op2, et, sizeof op1);
    if (n_rama)
        tam = 4u * (uint32_t)n_rama;    /* slt + beq/bne (+ addi) */
    else if (es_li_largo(s, reg_li, sizeof reg_li, &valor_li))
        tam = 8;                        /* se expande a dos instrucciones */
    c->pc_text += tam;
    if (c->pasada == 1)
        return;
    if (pc + tam > DATA_BASE) {
        error_linea(c, linea, "el programa no cabe en el segmento .text (máximo %u instrucciones)",
                    (unsigned)MAX_INSTR_PROGRAMA);
        return;
    }

    if (n_rama) {
        rama_comparacion(c, linea, pc, n_rama, rama, op1, op2, et);
        return;
    }
    if (tam == 8) {
        li_largo(c, linea, pc, reg_li, valor_li);
        return;
    }
    snprintf(texto, sizeof texto, "%s", s);

    mn = s;
    for (ops = s; *ops && !isspace((unsigned char)*ops); ops++)
        *ops = (char)tolower((unsigned char)*ops);
    if (*ops)
        *ops++ = '\0';

    if (!buscar_mnemonico(c->isa, mn)) {
        error_linea(c, linea, "instrucción desconocida '%s' (no está definida en el ISA)", mn);
        return;
    }

    /* elegir la sintaxis según el número de operandos escritos */
    n = dividir(ops, partes, 4);
    d = buscar_forma(c->isa, mn, n);
    if (!d) {
        formas[0] = '\0';
        for (i = 0; i < c->isa->n; i++)
            if (strcmp(c->isa->inst[i].mnemonico, mn) == 0)
                snprintf(formas + strlen(formas), sizeof formas - strlen(formas), "%s%s %s",
                         formas[0] ? "  |  " : "", mn, c->isa->inst[i].operandos);
        error_linea(c, linea, "'%s' no acepta %d operando(s); formas válidas: %s", mn, n, formas);
        return;
    }
    strcpy(patron, d->operandos);
    m = dividir(patron, tok, 6);

    for (i = 0, j = 0; i < m && ok; i++) {
        /* campo implícito, p. ej. rd=$v0: no consume un operando escrito */
        if ((extra = strchr(tok[i], '=')) != NULL) {
            *extra = '\0';
            reg = (uint32_t)numero_registro(extra + 1);
            if (strcmp(tok[i], "rd") == 0)      rd = reg;
            else if (strcmp(tok[i], "rs") == 0) rs = reg;
            else                                rt = reg;
            continue;
        }
        p = partes[j++];
        /* un registro escrito que va a dos campos, p. ej. rd+rs */
        if ((extra = strchr(tok[i], '+')) != NULL) {
            *extra++ = '\0';
            ok = registro(c, linea, p, &reg);
            if (strcmp(tok[i], "rd") == 0 || strcmp(extra, "rd") == 0) rd = reg;
            if (strcmp(tok[i], "rs") == 0 || strcmp(extra, "rs") == 0) rs = reg;
            if (strcmp(tok[i], "rt") == 0 || strcmp(extra, "rt") == 0) rt = reg;
        } else if (strcmp(tok[i], "rd") == 0) {
            ok = registro(c, linea, p, &rd);
        } else if (strcmp(tok[i], "rs") == 0) {
            ok = registro(c, linea, p, &rs);
        } else if (strcmp(tok[i], "rt") == 0) {
            ok = registro(c, linea, p, &rt);
        } else if (strcmp(tok[i], "shamt") == 0) {
            ok = valor(c, linea, p, &v);
            if (ok && (v < 0 || v > 31)) {
                error_linea(c, linea, "el desplazamiento debe estar entre 0 y 31");
                ok = 0;
            }
            shamt = (uint32_t)v;
        } else if (strcmp(tok[i], "imm") == 0) {
            ok = inmediato(c, linea, p, -32768, 65535, &imm);
        } else if (strcmp(tok[i], "label") == 0) {
            if (!es_identificador(p) || !(e = buscar_etiqueta(c->p, p))) {
                error_linea(c, linea, "la etiqueta '%s' no está definida", p);
                ok = 0;
            } else if (d->formato == FMT_J) {
                dir = (e->dir >> 2) & 0x03FFFFFF;           /* direccionamiento pseudodirecto */
            } else {
                v = ((long long)e->dir - (long long)(pc + 4)) / 4;   /* relativo al PC */
                imm = (uint32_t)v & 0xFFFF;
            }
        } else if (strcmp(tok[i], "imm(rs)") == 0) {
            par = strchr(p, '(');
            cierre = strrchr(p, ')');
            if (!par || !cierre || cierre < par || cierre[1] != '\0') {
                error_linea(c, linea, "se esperaba desplazamiento(registro), p. ej. 8($s0)");
                ok = 0;
                break;
            }
            *par = '\0';
            *cierre = '\0';
            desp = recortar(p);
            if (*desp == '\0')
                imm = 0;
            else
                ok = inmediato(c, linea, desp, -32768, 32767, &imm);
            if (ok)
                ok = registro(c, linea, recortar(par + 1), &rs);
        }
    }
    if (!ok)
        return;

    b = codificar(d, rs, rt, rd, shamt, imm, dir);
    escribir_palabra(c->cpu, pc, b);

    lf = &c->p->fuente[c->p->num_instr++];
    lf->dir = pc;
    lf->linea = linea;
    strcpy(lf->texto, texto);
}

static void procesar_linea(Ctx *c, char *linea, int num)
{
    char *s, *dp, nombre[128];
    int en_comillas = 0;
    size_t len;

    /* quitar comentario (respetando '#' dentro de 'c' o de "cadenas") */
    for (s = linea; *s; s++) {
        if (en_comillas && *s == '\\' && s[1])
            s++;                                    /* \" no cierra la cadena */
        else if ((*s == '\'' || *s == '"') && (!en_comillas || *s == en_comillas))
            en_comillas = en_comillas ? 0 : *s;
        else if (*s == '#' && !en_comillas) {
            *s = '\0';
            break;
        }
    }
    s = recortar(linea);

    /* etiquetas al inicio de la línea */
    while ((dp = strchr(s, ':')) != NULL) {
        len = (size_t)(dp - s);
        if (len >= sizeof nombre)
            break;
        memcpy(nombre, s, len);
        nombre[len] = '\0';
        if (!es_identificador(recortar(nombre)))
            break;
        definir_etiqueta(c, num, recortar(nombre));
        s = recortar(dp + 1);
    }

    if (*s == '\0')
        return;
    if (*s == '.')
        directiva(c, num, s);
    else
        instruccion(c, num, s);
}

int ensamblar(const char *ruta, const ISA *isa, CPU *cpu, Programa *p)
{
    FILE *f;
    char linea[256];
    int num;
    Ctx c;

    memset(p, 0, sizeof *p);
    snprintf(p->ruta, sizeof p->ruta, "%s", ruta);
    f = fopen(ruta, "r");
    if (!f) {
        error_general(p, "error: no se pudo abrir el programa '%s'", ruta);
        return 0;
    }

    c.isa = isa;
    c.cpu = cpu;
    c.p = p;
    for (c.pasada = 1; c.pasada <= 2; c.pasada++) {
        rewind(f);
        c.sec = SEC_TEXT;
        c.pc_text = TEXT_BASE;
        c.pc_data = DATA_BASE;
        num = 0;
        while (fgets(linea, sizeof linea, f)) {
            num++;
            procesar_linea(&c, linea, num);
        }
        p->text_fin = c.pc_text;
        p->data_fin = c.pc_data;
    }
    fclose(f);
    return p->num_errores == 0;
}
