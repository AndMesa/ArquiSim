# Programa 04 -- add vs addu, sub vs subu, addi vs addiu
#
# Las versiones con "u" hacen la MISMA operación, pero si el resultado no cabe
# en un entero de 32 bits con signo NO detienen el procesador: el valor da la vuelta.
# Ojo: en addiu el inmediato también se extiende con signo (-4 = 0xFFFC).
# Necesita: addu, subu, addiu (+ lui, ori)

.text
        lui   $t0, 0x7FFF
        ori   $t0, $t0, 0xFFFF      # 0x7FFFFFFF: el mayor entero positivo
        addiu $t1, $t0, 1           # 0x80000000: da la vuelta SIN excepción
        addu  $t2, $t0, $t0         # 0xFFFFFFFE
        subu  $t3, $zero, $t0       # 0 - 0x7FFFFFFF = 0x80000001
        addiu $t4, $zero, -4        # 0xFFFFFFFC
        addiu $sp, $sp, -4          # uso típico: mover un puntero (0x400 -> 0x3FC)
