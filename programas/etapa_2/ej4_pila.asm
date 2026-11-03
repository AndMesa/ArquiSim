# Ejercicio 4 -- La pila a mano: guardar y recuperar registros
# Instrucciones: addiu, sw, lw (+ las de la Etapa 1)
#
# $sp empieza en 0x400 y la pila crece HACIA ABAJO.
#
# 1. "Push": reserve 2 palabras (addiu $sp, $sp, -8) y guarde $s0 y $s1 en ellas.
# 2. Destruya $s0 y $s1 (póngalos en 0): simula que otra parte del programa los usa.
# 3. "Pop": recupere $s0 y $s1 desde la pila y devuelva $sp a su valor inicial.
#
# Preguntas: ¿en qué direcciones quedaron guardados $s0 y $s1? ¿Qué pasa con esas
# palabras después del pop: se borran? Mire la ventana PILA en cada paso.
#
# Complete ej4_pila.esperado y verifique con:
#   ./sim -t programas/etapa_2/ej4_pila.asm
.text
        addi $s0, $zero, 111
        addi $s1, $zero, 222
# escriba aquí su código

