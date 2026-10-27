/*
 * cpu.c -- Herramientas de acceso a registros y memoria.
 */
#include <stdarg.h>
#include <stdio.h>
#include "cpu.h"

void escribir_registro(CPU *cpu, uint32_t r, uint32_t valor)
{
    if (r != 0 && r < NUM_REGISTROS)    /* $zero está cableado a 0 */
        cpu->reg[r] = valor;
}

Resultado excepcion(CPU *cpu, const char *formato, ...)
{
    va_list ap;
    va_start(ap, formato);
    vsnprintf(cpu->excepcion, sizeof cpu->excepcion, formato, ap);
    va_end(ap);
    return EXCEPCION;
}

static Resultado validar(CPU *cpu, uint32_t dir, uint32_t tam)
{
    if (dir > MEM_BYTES - tam)
        return excepcion(cpu, "dirección 0x%X fuera del espacio de direcciones (0x000 - 0x%03X)",
                         dir, MEM_BYTES - 1);
    if (dir % tam != 0)
        return excepcion(cpu, "dirección 0x%03X no está alineada a %u bytes", dir, tam);
    return OK;
}

/* Los BOTONES son de entrada: las escrituras del programa no los cambian. */
static void escribir_ram(CPU *cpu, uint32_t dir, uint8_t valor)
{
    if (dir >= BOTONES_DIR && dir < BOTONES_DIR + 4)
        return;
    cpu->ram[dir] = valor;
}

void rgb_escribir_color(CPU *cpu, uint32_t color)
{
    uint32_t pos = palabra_en(cpu, RGB_POS_DIR), x = pos & 0xFFFF, y = pos >> 16;

    if (x >= RGB_ANCHO || y >= RGB_ALTO)
        return;                         /* posición fuera de la pantalla: se ignora */
    cpu->rgb[y][x] = color & 0xFFFFFF;
    if (++x >= RGB_ANCHO) {             /* autoincremento: siguiente píxel, siguiente fila */
        x = 0;
        if (++y >= RGB_ALTO)
            y = 0;
    }
    poner_palabra(cpu, RGB_POS_DIR, (y << 16) | x);
}

void cambiar_botones(CPU *cpu, uint32_t botones)
{
    poner_palabra(cpu, BOTONES_DIR, botones & 0xFF);
}

void poner_palabra(CPU *cpu, uint32_t dir, uint32_t valor)
{
    cpu->ram[dir]     = (uint8_t)(valor >> 24);
    cpu->ram[dir + 1] = (uint8_t)(valor >> 16);
    cpu->ram[dir + 2] = (uint8_t)(valor >> 8);
    cpu->ram[dir + 3] = (uint8_t)valor;
}

uint32_t palabra_en(const CPU *cpu, uint32_t dir)
{
    return (uint32_t)cpu->ram[dir] << 24 | (uint32_t)cpu->ram[dir + 1] << 16 |
           (uint32_t)cpu->ram[dir + 2] << 8 | (uint32_t)cpu->ram[dir + 3];
}

Resultado leer_instruccion(CPU *cpu, uint32_t dir, uint32_t *valor)
{
    if (validar(cpu, dir, 4) != OK)
        return EXCEPCION;
    *valor = palabra_en(cpu, dir);
    return OK;
}

Resultado leer_palabra(CPU *cpu, uint32_t dir, uint32_t *valor)
{
    if (validar(cpu, dir, 4) != OK)
        return EXCEPCION;
    *valor = palabra_en(cpu, dir);
    cpu->est.lecturas++;
    return OK;
}

Resultado escribir_palabra(CPU *cpu, uint32_t dir, uint32_t valor)
{
    if (validar(cpu, dir, 4) != OK)
        return EXCEPCION;
    escribir_ram(cpu, dir,     (uint8_t)(valor >> 24));
    escribir_ram(cpu, dir + 1, (uint8_t)(valor >> 16));
    escribir_ram(cpu, dir + 2, (uint8_t)(valor >> 8));
    escribir_ram(cpu, dir + 3, (uint8_t)valor);
    cpu->est.escrituras++;
    if (dir == RGB_COLOR_DIR)           /* el dispositivo reacciona a la escritura */
        rgb_escribir_color(cpu, valor);
    return OK;
}

Resultado leer_byte(CPU *cpu, uint32_t dir, uint8_t *valor)
{
    if (validar(cpu, dir, 1) != OK)
        return EXCEPCION;
    *valor = cpu->ram[dir];
    cpu->est.lecturas++;
    return OK;
}

Resultado escribir_byte(CPU *cpu, uint32_t dir, uint8_t valor)
{
    if (validar(cpu, dir, 1) != OK)
        return EXCEPCION;
    escribir_ram(cpu, dir, valor);
    cpu->est.escrituras++;
    return OK;
}

static void banderas_zn(CPU *cpu, uint32_t r, uint32_t c, uint32_t v)
{
    cpu->banderas = (r == 0 ? BANDERA_Z : 0) | (r >> 31 ? BANDERA_N : 0) |
                    (c ? BANDERA_C : 0) | (v ? BANDERA_V : 0);
}

uint32_t sumar_con_banderas(CPU *cpu, uint32_t a, uint32_t b)
{
    uint32_t r = a + b;

    /* C: hubo acarreo del bit 31.  V: dos operandos del mismo signo dan otro signo. */
    banderas_zn(cpu, r, r < a, (~(a ^ b) & (a ^ r)) >> 31);
    return r;
}

uint32_t restar_con_banderas(CPU *cpu, uint32_t a, uint32_t b)
{
    uint32_t r = a - b;

    /* C: hubo préstamo (a < b sin signo).  V: operandos de distinto signo y el
     * resultado tiene el signo de b. */
    banderas_zn(cpu, r, a < b, ((a ^ b) & (a ^ r)) >> 31);
    return r;
}

uint32_t banderas_logicas(CPU *cpu, uint32_t r)
{
    banderas_zn(cpu, r, 0, 0);
    return r;
}

int desborda_suma(int32_t a, int32_t b)
{
    int64_t r = (int64_t)a + (int64_t)b;
    return r > INT32_MAX || r < INT32_MIN;
}

int desborda_resta(int32_t a, int32_t b)
{
    int64_t r = (int64_t)a - (int64_t)b;
    return r > INT32_MAX || r < INT32_MIN;
}
