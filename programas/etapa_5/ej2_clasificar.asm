# Ejercicio 2 -- Clasificar números con banderas (como en el PDUA)
# Instrucciones: cmp, jz, jn (+ las anteriores)
#
#   int N = 7;                                  // 0x200
#   int A[7] = {4, 0, -3, 7, 0, -1, 9};         // desde 0x204
#   int neg, ceros, pos;                        // 0x220, 0x224, 0x228
#   para cada A[i]: si es negativo neg++; si es cero ceros++; si no pos++
#
# Use cmp $t0, $zero (calcula A[i] - 0 solo para las banderas) y luego jz / jn.
# OJO: cualquier instrucción aritmética entre el cmp y el salto cambia las banderas.
#
# Preguntas: ¿qué valen Z, N, C y V después de comparar 0, -3 y 7? ¿Por qué la
# advertencia sobre poner una addi entre cmp y jz? ¿Cómo haría lo mismo sin
# banderas (con slt y beq)? ¿Cuál versión usa menos instrucciones?
#
# Complete ej2_clasificar.esperado y verifique con:
#   ./sim -t programas/etapa_5/ej2_clasificar.asm
.data
N:      .word 7
A:      .word 4, 0, -3, 7, 0, -1, 9
neg:    .word 0
ceros:  .word 0
pos:    .word 0
.text
# escriba aquí su código

