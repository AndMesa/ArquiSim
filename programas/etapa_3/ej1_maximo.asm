# Ejercicio 1 -- if-else: máximo y diferencia
# Instrucciones: slt, beq, bne, j (+ las de las Etapas 1 y 2)
#
#   int a = 17, b = 42;             // en 0x200 y 0x204
#   int max, dif;                   // en 0x208 y 0x20C
#   if (a < b) { max = b; dif = b - a; }
#   else       { max = a; dif = a - b; }
#
# Recuerde la lógica negativa: se salta cuando la condición NO se cumple.
#
# Preguntas: cambie los valores de a y b (también iguales). ¿Siempre da bien?
# ¿Cuántas instrucciones se ejecutan por cada camino? ¿Qué valor tiene el
# inmediato de su beq/bne y por qué?
#
# Complete ej1_maximo.esperado y verifique con:
#   ./sim -t programas/etapa_3/ej1_maximo.asm
.data
a:      .word 17
b:      .word 42
max:    .word 0
dif:    .word 0
.text
# escriba aquí su código

