/*
 * ciclo.c -- El ciclo de instrucción: Fetch -> Decode -> Execute.
 */
#include <string.h>
#include "ciclo.h"

int programa_terminado(const Programa *p, const CPU *cpu)
{
    return cpu->pc == p->text_fin || cpu->terminado;
}

int cargar_programa(const ISA *isa, const char *ruta, Programa *p, CPU *cpu)
{
    int ok;

    memset(cpu, 0, sizeof *cpu);
    cpu->reg[REG_SP] = PILA_INICIAL;
    cpu->pc = TEXT_BASE;
    ok = ensamblar(ruta, isa, cpu, p);
    memset(&cpu->est, 0, sizeof cpu->est);     /* cargar el programa no cuenta */
    return ok;
}

/* Registros internos del camino de datos (multiciclo):
 *   A    <- reg[rs]
 *   B    <- reg[rt] (tipo R y saltos condicionales) o el inmediato extendido (tipo I)
 *   TEMP <- resultado de la ALU: dirección en lw/sw, rs - rt en beq/bne, destino
 *           en tipo J, o el valor que se escribió en el registro destino o en ACC */
static void cargar_operandos(CPU *cpu, const Instruccion *in)
{
    int salto = in->def && in->def->formato == FMT_I && strstr(in->def->operandos, "label");

    cpu->a = cpu->reg[in->rs];
    if (!in->def || in->def->formato == FMT_R || salto)
        cpu->b = cpu->reg[in->rt];
    else if (in->def->formato == FMT_I)
        cpu->b = (uint32_t)in->simm;
    else
        cpu->b = in->direccion << 2;
}

static void guardar_resultado(CPU *cpu, const Instruccion *in, uint32_t acc_antes, uint32_t pc)
{
    const char *ops = in->def->operandos;

    if (strstr(ops, "imm(rs)"))
        cpu->temp = cpu->a + (uint32_t)in->simm;                    /* dirección de memoria */
    else if (in->def->formato == FMT_I && strstr(ops, "label"))
        cpu->temp = cpu->a - cpu->b;                                /* comparación          */
    else if (in->def->formato == FMT_J)
        cpu->temp = ((pc + 4) & 0xF0000000u) | (in->direccion << 2); /* destino del salto    */
    else if (cpu->acc != acc_antes)
        cpu->temp = cpu->acc;
    else if (in->def->formato == FMT_R && strstr(ops, "rd"))
        cpu->temp = cpu->reg[in->rd];
    else if (in->def->formato == FMT_R && strstr(ops, "rs") && strstr(ops, "rt"))
        cpu->temp = cpu->a - cpu->b;                                /* cmp                  */
    else if (in->def->formato == FMT_I && strstr(ops, "rt"))
        cpu->temp = cpu->reg[in->rt];
}

EstadoPaso ejecutar_paso(const ISA *isa, const Programa *p, CPU *cpu, Instruccion *in)
{
    uint32_t pc = cpu->pc, acc_antes = cpu->acc;

    memset(in, 0, sizeof *in);
    if (programa_terminado(p, cpu))
        return PASO_FIN;
    cpu->espera_ms = 0;
    cpu->excepcion[0] = '\0';

    if (pc < TEXT_BASE || pc >= p->text_fin) {
        excepcion(cpu, "el PC (0x%X) salió del segmento .text", pc);
        return PASO_EXCEPCION;
    }

    /* FETCH: IR <- MEM[PC] */
    if (leer_instruccion(cpu, pc, &cpu->ir) != OK)
        return PASO_EXCEPCION;

    /* DECODE: separar campos, reconocer la instrucción y leer los operandos
     * del banco de registros en los registros internos A y B */
    decodificar(isa, cpu->ir, in);
    cpu->pc = pc + 4;
    cargar_operandos(cpu, in);

    /* EXECUTE */
    if (!in->def)
        excepcion(cpu, "código de operación desconocido (IR = 0x%08X)", cpu->ir);
    else
        ejecutar(cpu, in);
    cpu->reg[0] = 0;        /* $zero está cableado a 0 */
    if (in->def)
        guardar_resultado(cpu, in, acc_antes, pc);

    if (in->def) {
        cpu->est.instr[in->def->formato]++;
        if (cpu->pc != pc + 4)
            cpu->est.saltos++;
    }
    return cpu->excepcion[0] ? PASO_EXCEPCION : PASO_OK;
}
