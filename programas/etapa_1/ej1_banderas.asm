# Ejercicio 1 -- Suma, resta y banderas
# Instrucciones: add, addi, sub, addu, subu, addiu, lui, ori, and
#
# PARTE A: prediga ANTES de ejecutar el resultado y las banderas de cada línea
# marcada (A a E) y anótelos en la tabla. Luego verifique paso a paso con Enter.
#
#        Resultado      Z  N  C  V
#   A    __________     _  _  _  _
#   B    __________     _  _  _  _
#   C    __________     _  _  _  _
#   D    __________     _  _  _  _
#   E    __________     _  _  _  _
#
# PARTE B: con a = 7 en $s1 y b = 4 en $s2 calcule, SIN multiplicar,
#   f = 3a - 2b  en $s0
#   g = b - a    en $s3   (debe ser la última instrucción del programa)
#
# Complete ej1_banderas.esperado y verifique con:
#   ./sim -t programas/etapa_1/ej1_banderas.asm
.text
# ---------------- Parte A ----------------
        lui   $t0, 0x7FFF
        ori   $t0, $t0, 0xFFFF      # $t0 = 0x7FFFFFFF
        addu  $t1, $t0, $t0         # A
        addi  $t2, $zero, -1        # B
        addiu $t3, $t2, 1           # C
        subu  $t4, $zero, $t2       # D
        and   $t5, $t1, $zero       # E

# ---------------- Parte B ----------------
        addi  $s1, $zero, 7         # a
        addi  $s2, $zero, 4         # b
# escriba aquí su código

