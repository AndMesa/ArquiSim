/*
 * isa.c -- Lectura de isa.txt (el contrato) y decodificación de instrucciones.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "isa.h"

const char *const NOMBRES_REG[NUM_REGISTROS] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8",   "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

int numero_registro(const char *nombre)
{
    int i;
    long long n;

    if (nombre[0] != '$')
        return -1;
    for (i = 0; i < NUM_REGISTROS; i++)
        if (strcmp(nombre, NOMBRES_REG[i]) == 0)
            return i;
    if (isdigit((unsigned char)nombre[1]) && leer_numero(nombre + 1, &n) && n < NUM_REGISTROS)
        return (int)n;
    return -1;
}

int leer_numero(const char *s, long long *valor)
{
    int negativo = 0, base = 10, d;
    long long r = 0;

    if (*s == '+' || *s == '-') {
        negativo = (*s == '-');
        s++;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    } else if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        base = 2;
        s += 2;
    }
    if (*s == '\0')
        return 0;
    for (; *s; s++) {
        if (isdigit((unsigned char)*s))
            d = *s - '0';
        else if (isxdigit((unsigned char)*s))
            d = tolower((unsigned char)*s) - 'a' + 10;
        else
            return 0;
        if (d >= base)
            return 0;
        r = r * base + d;
        if (r > 0xFFFFFFFFLL)
            return 0;
    }
    *valor = negativo ? -r : r;
    return 1;
}

static int leer_bits(const char *s, int nbits, uint32_t *valor)
{
    int i;

    if ((int)strlen(s) != nbits)
        return 0;
    *valor = 0;
    for (i = 0; i < nbits; i++) {
        if (s[i] != '0' && s[i] != '1')
            return 0;
        *valor = (*valor << 1) | (uint32_t)(s[i] - '0');
    }
    return 1;
}

static int es_campo_registro(const char *t)
{
    return strcmp(t, "rd") == 0 || strcmp(t, "rs") == 0 || strcmp(t, "rt") == 0;
}

/* Operandos válidos:
 *   rd, rs, rt, imm, shamt, label, imm(rs)
 *   rd+rs     un registro escrito que va a dos campos (forma de 2 direcciones)
 *   rd=$v0    campo implícito: no se escribe (p. ej. un acumulador) */
static int operando_valido(char *tok)
{
    static const char *const simples[] = { "rd", "rs", "rt", "imm", "shamt", "label", "imm(rs)" };
    char *mas, *igual;
    size_t i;

    if ((igual = strchr(tok, '=')) != NULL) {
        *igual = '\0';
        i = (size_t)(es_campo_registro(tok) && numero_registro(igual + 1) >= 0);
        *igual = '=';
        return (int)i;
    }
    if ((mas = strchr(tok, '+')) != NULL) {
        *mas = '\0';
        i = (size_t)(es_campo_registro(tok) && es_campo_registro(mas + 1));
        *mas = '+';
        return (int)i;
    }
    for (i = 0; i < sizeof simples / sizeof simples[0]; i++)
        if (strcmp(tok, simples[i]) == 0)
            return 1;
    return 0;
}

static int patron_valido(const char *patron)
{
    char copia[MAX_PATRON];
    char *tok;

    if (patron[0] == '\0')
        return 1;
    strcpy(copia, patron);
    for (tok = strtok(copia, ","); tok; tok = strtok(NULL, ","))
        if (!operando_valido(tok))
            return 0;
    return 1;
}

int operandos_escritos(const DefInstruccion *d)
{
    const char *s = d->operandos;
    int n = 0, implicito = 0;

    if (*s == '\0')
        return 0;
    for (;; s++) {
        if (*s == '=')
            implicito = 1;
        if (*s == ',' || *s == '\0') {
            n += !implicito;
            implicito = 0;
        }
        if (*s == '\0')
            break;
    }
    return n;
}

int cargar_isa(const char *ruta, ISA *isa)
{
    FILE *f;
    char linea[256], mn[32], fmt[8], op[16], fu[16], ops[64], nombre[32];
    char *p;
    int num_linea = 0, errores = 0, k, i;
    DefInstruccion *d;

    f = fopen(ruta, "r");
    if (!f) {
        fprintf(stderr, "error: no se pudo abrir el archivo del ISA '%s'\n", ruta);
        return 0;
    }
    isa->n = 0;

#define ERROR_ISA(...) do { fprintf(stderr, "%s:%d: error: ", ruta, num_linea); \
                            fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); errores++; } while (0)

    while (fgets(linea, sizeof linea, f)) {
        num_linea++;
        if ((p = strchr(linea, '#')) != NULL)
            *p = '\0';
        ops[0] = nombre[0] = '\0';
        k = sscanf(linea, "%31s %7s %15s %15s %63s %31s", mn, fmt, op, fu, ops, nombre);
        if (k <= 0)
            continue;
        if (k < 4) {
            ERROR_ISA("se esperaba: mnemónico formato opcode funct operandos");
            continue;
        }
        if (isa->n >= MAX_INSTRUCCIONES_ISA) {
            ERROR_ISA("demasiadas instrucciones (máximo %d)", MAX_INSTRUCCIONES_ISA);
            break;
        }
        d = &isa->inst[isa->n];

        for (p = mn; *p; p++)
            *p = (char)tolower((unsigned char)*p);
        if (strlen(mn) >= MAX_MNEMONICO) {
            ERROR_ISA("mnemónico '%s' demasiado largo", mn);
            continue;
        }
        strcpy(d->mnemonico, mn);
        if (nombre[0] == '\0')
            strcpy(nombre, mn);
        for (p = nombre; *p; p++)
            *p = (char)tolower((unsigned char)*p);
        if (strlen(nombre) >= MAX_MNEMONICO) {
            ERROR_ISA("nombre interno '%s' demasiado largo", nombre);
            continue;
        }
        strcpy(d->nombre, nombre);

        if (strcmp(fmt, "R") == 0 || strcmp(fmt, "r") == 0)
            d->formato = FMT_R;
        else if (strcmp(fmt, "I") == 0 || strcmp(fmt, "i") == 0)
            d->formato = FMT_I;
        else if (strcmp(fmt, "J") == 0 || strcmp(fmt, "j") == 0)
            d->formato = FMT_J;
        else {
            ERROR_ISA("formato '%s' inválido (use R, I o J)", fmt);
            continue;
        }

        if (!leer_bits(op, 6, &d->opcode)) {
            ERROR_ISA("el opcode de '%s' debe tener 6 bits binarios", mn);
            continue;
        }

        if (strcmp(fu, "-") == 0) {
            d->funct = -1;
            if (d->formato == FMT_R) {
                ERROR_ISA("'%s' es tipo R y necesita un campo funct de 6 bits", mn);
                continue;
            }
        } else {
            uint32_t funct;
            if (d->formato != FMT_R) {
                ERROR_ISA("solo las instrucciones tipo R usan funct (use '-')");
                continue;
            }
            if (!leer_bits(fu, 6, &funct)) {
                ERROR_ISA("el funct de '%s' debe tener 6 bits binarios", mn);
                continue;
            }
            d->funct = (int)funct;
        }

        if (strcmp(ops, "-") == 0)
            ops[0] = '\0';
        if (strlen(ops) >= MAX_PATRON || !patron_valido(ops)) {
            ERROR_ISA("operandos '%s' inválidos (use rd, rs, rt, imm, shamt, label, imm(rs), "
                      "rd+rs o rd=$reg)", ops);
            continue;
        }
        strcpy(d->operandos, ops);

        for (i = 0; i < isa->n; i++) {
            const DefInstruccion *o = &isa->inst[i];
            int misma_cod = o->opcode == d->opcode && (d->formato != FMT_R || o->funct == d->funct);

            int mismo_nombre = strcmp(o->nombre, d->nombre) == 0;
            int mismo_mn = strcmp(o->mnemonico, d->mnemonico) == 0;

            /* Se permite: la misma instrucción con otra sintaxis (mismo nombre y
             * codificación), o el mismo mnemónico para otra instrucción (otro nombre
             * y codificación) si se escribe con distinto número de operandos. */
            if (mismo_mn && operandos_escritos(o) == operandos_escritos(d)) {
                ERROR_ISA("'%s' ya tiene una forma con %d operando(s)", mn, operandos_escritos(d));
                break;
            }
            if (mismo_nombre && (!misma_cod || o->formato != d->formato)) {
                ERROR_ISA("'%s' ya está definida con otra codificación", d->nombre);
                break;
            }
            if (!mismo_nombre && misma_cod) {
                ERROR_ISA("'%s' tiene la misma codificación que '%s'", d->nombre, o->nombre);
                break;
            }
        }
        if (i < isa->n)
            continue;
        isa->n++;
    }
#undef ERROR_ISA
    fclose(f);

    if (errores == 0 && isa->n == 0) {
        fprintf(stderr, "error: '%s' no define ninguna instrucción\n", ruta);
        errores++;
    }
    return errores == 0;
}

const DefInstruccion *buscar_mnemonico(const ISA *isa, const char *mnemonico)
{
    int i;

    for (i = 0; i < isa->n; i++)
        if (strcmp(isa->inst[i].mnemonico, mnemonico) == 0)
            return &isa->inst[i];
    return NULL;
}

const DefInstruccion *buscar_forma(const ISA *isa, const char *mnemonico, int escritos)
{
    int i;

    for (i = 0; i < isa->n; i++)
        if (strcmp(isa->inst[i].mnemonico, mnemonico) == 0 && operandos_escritos(&isa->inst[i]) == escritos)
            return &isa->inst[i];
    return NULL;
}

/* Si una instrucción tiene varias sintaxis, se usa la primera definida en isa.txt. */
void decodificar(const ISA *isa, uint32_t b, Instruccion *in)
{
    int i;
    const DefInstruccion *d;

    in->binario   = b;
    in->op        = b >> 26;
    in->rs        = (b >> 21) & 0x1F;
    in->rt        = (b >> 16) & 0x1F;
    in->rd        = (b >> 11) & 0x1F;
    in->shamt     = (b >> 6) & 0x1F;
    in->funct     = b & 0x3F;
    in->imm       = b & 0xFFFF;
    in->simm      = (in->imm & 0x8000) ? (int32_t)in->imm - 0x10000 : (int32_t)in->imm;
    in->direccion = b & 0x03FFFFFF;
    in->def       = NULL;

    for (i = 0; i < isa->n; i++) {
        d = &isa->inst[i];
        if (d->opcode == in->op && (d->formato != FMT_R || (uint32_t)d->funct == in->funct)) {
            in->def = d;
            return;
        }
    }
}
