# Ejercicio 2 -- for: sumar y contar negativos
# Instrucciones: slt, beq, bne, j (o blt, bge: pseudoinstrucciones) (+ Etapas 1 y 2)
#
#   int N = 6;                              // en 0x200
#   int A[6] = {3, -1, 4, 1, -5, 9};        // desde 0x204
#   int suma = 0, neg = 0;                  // en 0x21C y 0x220
#   for (i = 0; i < N; i++) {
#       suma = suma + A[i];
#       if (A[i] < 0) neg++;
#   }
#
# Preguntas: ¿cuántas veces se ejecuta el cuerpo? ¿Cuántas instrucciones en total
# (mírelo en el encabezado de la interfaz)? Si recorre A con un puntero que avanza
# de 4 en 4, ¿cuánto vale al final?
#
# Complete ej2_ciclo.esperado y verifique con:
#   ./sim -t programas/etapa_3/ej2_ciclo.asm
.data
N:      .word 6
A:      .word 3, -1, 4, 1, -5, 9
suma:   .word 0
neg:    .word 0
.text
# escriba aquí su código

