# Programa 08 -- Excepción por desbordamiento
# 0x7FFFFFFF es el mayor entero de 32 bits con signo: sumarle 1 desborda.
# addi debe detectar el desbordamiento y detener el procesador.
# Necesita: lui, ori

.text
        lui  $t0, 0x7FFF
        ori  $t0, $t0, 0xFFFF       # $t0 = 0x7FFFFFFF
        addi $t1, $t0, 1            # desbordamiento -> EXCEPCIÓN
        addi $t2, $zero, 99         # nunca se ejecuta
