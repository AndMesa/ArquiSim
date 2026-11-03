# Ejercicio 3 -- lb vs lbu: el mismo byte, dos números
# Instrucciones: lb, lbu, sb, sw (+ las de la Etapa 1)
#
# En V (0x200) hay 4 bytes: -5, 200, 0x7F, 0x80.
#
# 1. $s0 = lb de V[0]     $s1 = lbu de V[0]
#    $s2 = lb de V[1]     $s3 = lbu de V[1]
#    $s4 = lb de V[3]
#    Prediga cada valor (en decimal y en hexadecimal) ANTES de ejecutar.
# 2. Guarde $s2 completo con sw en W (0x204) y luego solo su byte bajo con sb en V[2].
#
# Preguntas: ¿por qué 200 se lee como -56 con lb? ¿Cuál usaría para leer un carácter
# de texto o el color de un píxel, y por qué? ¿Por qué no existe "sbu"?
#
# Complete ej3_signo.esperado y verifique con:
#   ./sim -t programas/etapa_2/ej3_signo.asm
.data
V:      .byte -5, 200, 0x7F, 0x80
W:      .word 0
.text
# escriba aquí su código

