# Ejercicio 1 -- Suma, resta y banderas
# Instrucciones: add, addi, sub, addu, subu, addiu, lui, ori, and
#
# PARTE A: prediga ANTES de ejecutar el resultado y las banderas de cada línea
# marcada (A a E) y anótelos en la tabla. Luego verifique paso a paso con Enter.
#
#        Resultado      Z  N  C  V
#   A    __________     _  _  _  _
#   B    __________     _  X  _  X
#   C    __________     _  _  X  X
#   D    __________     X  _  _  _
#   E    __________     X  _  X  _
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
	add   $s5,$s1,$s1
        add   $s5,$s5,$s1
        add   $s4,$s2,$s2
        nor    $s4,$s4,$zero
        addi   $s4,$s4,0x01
        add   $s0,$s5,$s4
	sub   $s3,$s2,$s1
# escriba aquí su código

