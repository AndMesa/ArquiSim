# Ejercicio 2 -- Big-endian: los bytes de una palabra
# Instrucciones: lbu, sb (+ las de la Etapa 1)
#
# X = 0x11223344 está en 0x200. ArquiSim es BIG-ENDIAN: el byte más significativo
# va en la dirección MENOR.
#
# 1. Cargue con lbu los bytes de 0x200, 0x201, 0x202 y 0x203 en $s0, $s1, $s2 y $s3.
#    Prediga cada valor ANTES de ejecutar.
# 2. Escriba en Y (0x204) la misma palabra en orden LITTLE-ENDIAN, byte por byte con sb
#    (el byte menos significativo en la dirección menor).
# 3. Lea Y como palabra con lw en $s4: ¿qué número ve un procesador big-endian?
#
# Preguntas: ¿cómo cambiaría su respuesta del punto 1 en un procesador little-endian
# (como x86)? ¿Por qué importa esto al enviar datos entre dos computadores?
#
# Complete ej2_endian.esperado y verifique con:
#   ./sim -t programas/etapa_2/ej2_endian.asm
.data
X:      .word 0x11223344
Y:      .space 4
.text
# escriba aquí su código

