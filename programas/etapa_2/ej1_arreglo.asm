# Ejercicio 1 -- Recorrer un arreglo "a mano" (sin ciclos todavía)
# Instrucciones: lw, sw (+ las de la Etapa 1)
#
#   int A[4] = {5, -3, 12, 7};      // en 0x200
#   int suma;                       // en 0x210
#   suma = A[0] + A[1] + A[2] + A[3];
#   intercambiar A[0] y A[3];
#
# 1. Ponga en $s0 la dirección base de A.
# 2. Calcule la suma en registros y guárdela en la variable SUMA (con sw).
# 3. Intercambie A[0] y A[3] EN MEMORIA.
#
# Preguntas: ¿qué desplazamiento usó para A[3] y por qué? Use "m A" en la interfaz para
# ver el arreglo antes y después. ¿Qué pasa si usa lw $t0, 2($s0)?
#
# Complete ej1_arreglo.esperado y verifique con:
#   ./sim -t programas/etapa_2/ej1_arreglo.asm
.data
A:      .word 5, -3, 12, 7
SUMA:   .word 0
.text
# escriba aquí su código

