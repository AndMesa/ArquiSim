/*
 * sistema.c -- Servicios del sistema (syscall): consola y control del programa.
 * En un computador real los atiende el sistema operativo.
 */
#include <stdio.h>
#include <string.h>
#include "sistema.h"

void consola_agregar(CPU *cpu, const char *texto)
{
    size_t n = strlen(texto), len = strlen(cpu->consola);

    if (n >= sizeof cpu->consola)
        texto += n - (sizeof cpu->consola / 2), n = sizeof cpu->consola / 2;
    if (len + n >= sizeof cpu->consola) {          /* olvidar la mitad más antigua */
        memmove(cpu->consola, cpu->consola + len / 2, len - len / 2 + 1);
        len = strlen(cpu->consola);
    }
    memcpy(cpu->consola + len, texto, n + 1);
}

Resultado llamar_sistema(CPU *cpu)
{
    uint32_t a0 = cpu->reg[4], dir;
    char buf[64], cadena[512];
    size_t n = 0;

    switch (cpu->reg[2]) {                          /* $v0: número de servicio */
    case 1:
        snprintf(buf, sizeof buf, "%d", (int32_t)a0);
        consola_agregar(cpu, buf);
        return OK;
    case 4:
        for (dir = a0; n + 1 < sizeof cadena; dir++) {
            if (dir >= MEM_BYTES)
                return excepcion(cpu, "syscall 4: la cadena en 0x%X se sale de la memoria", a0);
            if (cpu->ram[dir] == 0)
                break;
            cadena[n++] = (char)cpu->ram[dir];
        }
        cadena[n] = '\0';
        consola_agregar(cpu, cadena);
        return OK;
    case 5:
        cpu->pide_entero = 1;                       /* la interfaz pedirá el número */
        return OK;
    case 10:
        cpu->terminado = 1;
        return OK;
    case 11:
        buf[0] = (char)(a0 & 0xFF);
        buf[1] = '\0';
        consola_agregar(cpu, buf);
        return OK;
    case 32:
        cpu->espera_ms = a0;
        return OK;
    default:
        return excepcion(cpu, "syscall con $v0 = %u no está implementado", cpu->reg[2]);
    }
}
