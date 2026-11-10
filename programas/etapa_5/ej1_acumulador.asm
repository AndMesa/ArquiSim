# Ejercicio 1 -- La misma cuenta con 2 direcciones y con acumulador
# Instrucciones: add/sub de 2 direcciones, add/sub de 1 dirección, mtacc, mfacc
#
#   S = a + b + c + d - e      con a..e en $t1..$t5 = 10, 20, 30, 40, 5
#
# 1. En $s0, con instrucciones de 2 DIRECCIONES (add $s0, $t1 ...).
# 2. En $s1, con el ACUMULADOR (1 dirección): cargue ACC con mtacc, opere con
#    add/sub de 1 dirección y saque el resultado con mfacc.
#
# Preguntas: ¿cuántas instrucciones usó en cada forma? Compare el código máquina
# de add $s0, $t2 con el de add $t2: ¿cuál es una instrucción nueva y cuál solo
# una sintaxis nueva? ¿Qué banderas deja la última operación del acumulador?
#
# Complete ej1_acumulador.esperado y verifique con:
#   ./sim -t programas/etapa_5/ej1_acumulador.asm
.text
        addi $t1, $zero, 10         # a
        addi $t2, $zero, 20         # b
        addi $t3, $zero, 30         # c
        addi $t4, $zero, 40         # d
        addi $t5, $zero, 5          # e
# escriba aquí su código

