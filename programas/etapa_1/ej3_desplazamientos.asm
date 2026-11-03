# Ejercicio 3 -- Multiplicar y dividir con desplazamientos
# Instrucciones: sll, srl, sra, add, sub, addi
#
# Con x = 37 en $s0, calcule SIN mult (solo desplazamientos, sumas y restas):
#   $s1 = 10x        pista: 10x = 8x + 2x
#   $s2 = 7x
# Con y = -20 en $s3 y z = -7 en $s4:
#   $s5 = y sra 2
#   $s6 = y srl 2
#   $s7 = z sra 1
#
# Preguntas: ¿por qué $s6 da un número enorme? ¿Hacia dónde redondea sra (-7/2)?
#
# Complete ej3_desplazamientos.esperado y verifique con:
#   ./sim -t programas/etapa_1/ej3_desplazamientos.asm
.text
        addi $s0, $zero, 37         # x
        addi $s3, $zero, -20        # y
        addi $s4, $zero, -7         # z
# escriba aquí su código

