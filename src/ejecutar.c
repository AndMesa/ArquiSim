/*
 * ejecutar.c -- LA MICROARQUITECTURA: aquí se implementa lo que hace cada instrucción.
 *
 * isa.txt dice QUÉ instrucciones existen y cómo se codifican (el contrato).
 * Este archivo dice QUÉ HACE el hardware con cada una.
 *
 * Cuando se llama ejecutar(), la instrucción ya fue traída de memoria (fetch)
 * y decodificada. Usted tiene disponible:
 *
 *   in->rs, in->rt, in->rd   números de registro (0..31)
 *   in->shamt                cantidad de desplazamiento (0..31)
 *   in->imm                  inmediato de 16 bits SIN extender      (0 .. 65535)
 *   in->simm                 inmediato con extensión de signo   (-32768 .. 32767)
 *   in->direccion            campo de 26 bits de las instrucciones tipo J
 *   in->def->nombre          nombre de la instrucción en isa.txt ("add", "addi", ...)
 *
 *   cpu->reg[n]              valor del registro n
 *   cpu->acc                 acumulador (registro fuera del banco)
 *   cpu->pc                  YA apunta a la siguiente instrucción (PC + 4)
 *
 * Herramientas (ver cpu.h):
 *   escribir_registro(cpu, r, valor)      escribe un registro ($zero se ignora)
 *   leer_palabra(cpu, dir, &valor)        lee 32 bits de la RAM
 *   escribir_palabra(cpu, dir, valor)     escribe 32 bits en la RAM
 *   leer_byte / escribir_byte             acceso de 8 bits
 *   desborda_suma(a, b), desborda_resta(a, b)
 *   sumar_con_banderas(cpu, a, b)         a + b, actualizando las banderas Z N C V
 *   restar_con_banderas(cpu, a, b)        a - b, actualizando las banderas Z N C V
 *   banderas_logicas(cpu, r)              Z y N según r (C = V = 0); para and, or, ...
 *   excepcion(cpu, "mensaje", ...)        detiene el procesador con un error
 *
 * Las funciones de memoria devuelven EXCEPCION si la dirección es inválida;
 * en ese caso devuelva ese mismo resultado:
 *       if (leer_palabra(cpu, dir, &valor) != OK) return EXCEPCION;
 */
#include <string.h>
#include "cpu.h"

static int es(const Instruccion *in, const char *mnemonico)
{
    return strcmp(in->def->nombre, mnemonico) == 0;
}

Resultado ejecutar(CPU *cpu, const Instruccion *in)
{
    /* ---------------- add rd, rs, rt ---------------- */
    if (es(in, "add")) {
        uint32_t r = sumar_con_banderas(cpu, cpu->reg[in->rs], cpu->reg[in->rt]);
        if (cpu->banderas & BANDERA_V)          /* desbordamiento con signo */
            return excepcion(cpu, "desbordamiento aritmético en add");
        escribir_registro(cpu, in->rd, r);
        return OK;
    }

    /* ---------------- addi rt, rs, imm ---------------- */
    if (es(in, "addi")) {
        /* el inmediato es con signo: in->simm */
        uint32_t r = sumar_con_banderas(cpu, cpu->reg[in->rs], (uint32_t)in->simm);
        if (cpu->banderas & BANDERA_V)
            return excepcion(cpu, "desbordamiento aritmético en addi");
        escribir_registro(cpu, in->rt, r);
        return OK;
    }

    /*
     * TODO: para cada instrucción nueva
     *   1. agregue su línea en isa.txt (formato, opcode, funct, operandos)
     *   2. implemente aquí lo que hace
     *
     * Orden sugerido (cada paso tiene un programa de prueba en programas/):
     *   01_aritmetica   sub, addu, addiu, subu
     *   02_logica       and, or, xor, nor, andi, ori, xori, lui, sll, srl, sra
     *   03_memoria      lw, sw
     *   04_if           bne, beq, j
     *   05_ciclo        slt, sltu, slti
     *   06_funcion      jal, jr
     *   07_texto        lb, lbu, sb
     *   08_excepcion    (verifica que add/addi detecten el desbordamiento)
     *   09_pantalla     (dibuja en la pantalla mapeada en memoria: 0x400 + 32*y + x)
     *   10_direcciones  formas de 2 direcciones y add de 1 dirección (adda)
     *   11_acumulador   mtacc, mfacc, suba, addia  (instrucciones con el acumulador ACC)
     *   12_pseudo       move, li, la, nop, clear, b (solo líneas en isa.txt: no se implementan aquí)
     *   13_botones      (lee los botones en 0x600 por sondeo y los copia a la pantalla)
     *   14_pila         (push/pop de $ra con sw/lw y $sp)
     *   15_banderas     cmp, jz, jn, jc, jv; add/sub/addi de 1 dirección actualizan Z N C V
     *                   (use sumar_con_banderas / restar_con_banderas de cpu.h)
     *   16_datos        (variables y arreglos en .data: máximo y suma de un arreglo)
     *   17_interrupciones  ei, di, eret, mfepc  (cpu->ie, cpu->epc; el salto a la ISR
     *                   lo hace el hardware en ciclo.c)
     *   18_pantalla_rgb (pantalla con registros de comando en 0x610 y 0x614)
     *   19_mult_div     mult, multu, div, divu, mfhi, mflo, mul (cpu->hi, cpu->lo)
     *                   + blt/bgt/ble/bge (las expande el ensamblador: no se implementan aquí)
     *   20_consola      syscall: llame a llamar_sistema(cpu) (ver sistema.h)
     *
     * Pruebe con:  make test
     */

    return excepcion(cpu, "instrucción '%s' no implementada (ver src/ejecutar.c)",
                     in->def->nombre);
}
