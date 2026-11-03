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

    /* ================ Etapa 1: aritméticas y lógicas ================ */

    /* ---------------- sub rd, rs, rt ---------------- */
    if (es(in, "sub")) {
        uint32_t r = restar_con_banderas(cpu, cpu->reg[in->rs], cpu->reg[in->rt]);
        if (cpu->banderas & BANDERA_V)
            return excepcion(cpu, "desbordamiento aritmético en sub");
        escribir_registro(cpu, in->rd, r);
        return OK;
    }

    /* ---------------- addu, subu, addiu: igual, pero SIN excepción ---------------- */
    if (es(in, "addu")) {
        escribir_registro(cpu, in->rd, sumar_con_banderas(cpu, cpu->reg[in->rs], cpu->reg[in->rt]));
        return OK;
    }
    if (es(in, "subu")) {
        escribir_registro(cpu, in->rd, restar_con_banderas(cpu, cpu->reg[in->rs], cpu->reg[in->rt]));
        return OK;
    }
    if (es(in, "addiu")) {
        /* a pesar de la "u", el inmediato se extiende con signo */
        escribir_registro(cpu, in->rt, sumar_con_banderas(cpu, cpu->reg[in->rs], (uint32_t)in->simm));
        return OK;
    }

    /* ---------------- lógicas tipo R: rd = rs op rt ---------------- */
    if (es(in, "and")) {
        escribir_registro(cpu, in->rd, banderas_logicas(cpu, cpu->reg[in->rs] & cpu->reg[in->rt]));
        return OK;
    }
    if (es(in, "or")) {
        escribir_registro(cpu, in->rd, banderas_logicas(cpu, cpu->reg[in->rs] | cpu->reg[in->rt]));
        return OK;
    }
    if (es(in, "xor")) {
        escribir_registro(cpu, in->rd, banderas_logicas(cpu, cpu->reg[in->rs] ^ cpu->reg[in->rt]));
        return OK;
    }
    if (es(in, "nor")) {
        escribir_registro(cpu, in->rd, banderas_logicas(cpu, ~(cpu->reg[in->rs] | cpu->reg[in->rt])));
        return OK;
    }

    /* ---------------- lógicas tipo I: el inmediato se extiende con CEROS (in->imm) ---------------- */
    if (es(in, "andi")) {
        escribir_registro(cpu, in->rt, banderas_logicas(cpu, cpu->reg[in->rs] & in->imm));
        return OK;
    }
    if (es(in, "ori")) {
        escribir_registro(cpu, in->rt, banderas_logicas(cpu, cpu->reg[in->rs] | in->imm));
        return OK;
    }
    if (es(in, "xori")) {
        escribir_registro(cpu, in->rt, banderas_logicas(cpu, cpu->reg[in->rs] ^ in->imm));
        return OK;
    }

    /* ---------------- lui rt, imm: el inmediato va a los 16 bits altos ---------------- */
    if (es(in, "lui")) {
        escribir_registro(cpu, in->rt, in->imm << 16);
        return OK;
    }

    /* ---------------- desplazamientos: operan sobre rt, cantidad en shamt ---------------- */
    if (es(in, "sll")) {
        escribir_registro(cpu, in->rd, cpu->reg[in->rt] << in->shamt);
        return OK;
    }
    if (es(in, "srl")) {
        escribir_registro(cpu, in->rd, cpu->reg[in->rt] >> in->shamt);     /* rellena con ceros */
        return OK;
    }
    if (es(in, "sra")) {
        uint32_t x = cpu->reg[in->rt];
        /* copia el bit de signo: si x es negativo, desplazar el complemento y volver a complementar */
        escribir_registro(cpu, in->rd, (x & 0x80000000u) ? ~(~x >> in->shamt) : x >> in->shamt);
        return OK;
    }

    /* ================ Etapa 2: memoria ================ */
    /* Todas usan base + desplazamiento: dirección = rs + imm (imm con signo). */

    /* ---------------- lw rt, imm(rs) / sw rt, imm(rs): palabras de 32 bits ---------------- */
    if (es(in, "lw")) {
        uint32_t valor;
        if (leer_palabra(cpu, cpu->reg[in->rs] + (uint32_t)in->simm, &valor) != OK)
            return EXCEPCION;               /* fuera de rango o no alineada */
        escribir_registro(cpu, in->rt, valor);
        return OK;
    }
    if (es(in, "sw"))
        return escribir_palabra(cpu, cpu->reg[in->rs] + (uint32_t)in->simm, cpu->reg[in->rt]);

    /* ---------------- lb, lbu, sb: bytes ---------------- */
    if (es(in, "lb") || es(in, "lbu")) {
        uint8_t byte;
        if (leer_byte(cpu, cpu->reg[in->rs] + (uint32_t)in->simm, &byte) != OK)
            return EXCEPCION;
        if (es(in, "lb"))                   /* extiende el signo: copia el bit 7 hacia la izquierda */
            escribir_registro(cpu, in->rt, (uint32_t)(int32_t)(int8_t)byte);
        else                                /* lbu: rellena con ceros */
            escribir_registro(cpu, in->rt, byte);
        return OK;
    }
    if (es(in, "sb"))                       /* guarda los 8 bits bajos */
        return escribir_byte(cpu, cpu->reg[in->rs] + (uint32_t)in->simm, (uint8_t)cpu->reg[in->rt]);

    /* ================ Etapa 3: comparaciones y saltos ================ */
    /* cpu->pc ya vale PC + 4: los saltos relativos se cuentan desde ahí. */

    /* ---------------- comparaciones: escriben 1 o 0, no cambian las banderas ---------------- */
    if (es(in, "slt")) {
        escribir_registro(cpu, in->rd, (int32_t)cpu->reg[in->rs] < (int32_t)cpu->reg[in->rt]);
        return OK;
    }
    if (es(in, "sltu")) {
        escribir_registro(cpu, in->rd, cpu->reg[in->rs] < cpu->reg[in->rt]);
        return OK;
    }
    if (es(in, "slti")) {
        escribir_registro(cpu, in->rt, (int32_t)cpu->reg[in->rs] < in->simm);
        return OK;
    }

    /* ---------------- beq, bne: saltos condicionales relativos al PC ---------------- */
    if (es(in, "beq")) {
        if (cpu->reg[in->rs] == cpu->reg[in->rt])
            cpu->pc += (uint32_t)in->simm << 2;     /* imm cuenta instrucciones: x4 */
        return OK;
    }
    if (es(in, "bne")) {
        if (cpu->reg[in->rs] != cpu->reg[in->rt])
            cpu->pc += (uint32_t)in->simm << 2;
        return OK;
    }

    /* ---------------- j, jal: saltos pseudodirectos (tipo J) ---------------- */
    if (es(in, "jal"))
        escribir_registro(cpu, REG_RA, cpu->pc);    /* dirección de retorno: PC + 4 */
    if (es(in, "j") || es(in, "jal")) {
        cpu->pc = (cpu->pc & 0xF0000000u) | (in->direccion << 2);
        return OK;
    }

    /* ---------------- jr rs: saltar a la dirección guardada en un registro ---------------- */
    if (es(in, "jr")) {
        cpu->pc = cpu->reg[in->rs];
        return OK;
    }

    /*
     * TODO: para cada instrucción nueva
     *   1. agregue su línea en isa.txt (formato, opcode, funct, operandos)
     *   2. implemente aquí lo que hace
     *
     * Orden sugerido (cada etapa tiene programas de prueba en programas/ejemplos/):
     *
     *   Etapa 1 - Aritméticas y lógicas                         (YA IMPLEMENTADA)
     *     01_aritmetica   sub
     *     02_logica       lui, ori, and, andi, nor, sll, srl, sra
     *     03_xor          or, xor, xori
     *     04_sin_signo    addu, subu, addiu
     *     05_excepcion    (add/addi deben detectar el desbordamiento)
     *   Etapa 2 - Memoria                                       (YA IMPLEMENTADA)
     *     06_memoria      lw, sw
     *     07_bytes        lb, lbu, sb
     *   Etapa 3 - Control de flujo                              (YA IMPLEMENTADA)
     *     08_if           bne, j
     *     09_ciclo        slt
     *     10_comparaciones sltu, slti, beq
     *     11_datos, 12_texto            (integran memoria y ciclos)
     *     13_funcion      jal, jr
     *     14_pila         (push/pop de $ra con sw/lw y $sp)
     *     15_pseudo       move, li, la, nop, clear, b, not: solo líneas en isa.txt
     *   Etapa 4 - E/S mapeada en memoria (sin instrucciones nuevas)
     *     16_pantalla, 17_botones, 18_pantalla_rgb
     *   Etapa 5 - Extensiones del ISA
     *     19_direcciones  formas de 2 direcciones y add de 1 dirección (adda)
     *     20_acumulador   mtacc, mfacc, suba, addia
     *     21_banderas     cmp, jz, jn, jc, jv
     *     22_mult_div     mult, multu, div, divu, mfhi, mflo, mul (+ blt, ble: solo isa.txt)
     *     23_consola      syscall: llame a llamar_sistema(cpu) (ver sistema.h)
     *
     * Pruebe con:  make test
     */

    return excepcion(cpu, "instrucción '%s' no implementada (ver src/ejecutar.c)",
                     in->def->nombre);
}
