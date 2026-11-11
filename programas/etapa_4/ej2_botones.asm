# Ejercicio 2 -- Contar botones activados (sondeo)
# Instrucciones: las de las Etapas 1 a 3
#
# Los 8 botones son los bits 7..0 de la palabra en 0x600 (1 = activado).
#
# 1. Función CONTAR: $a0 = valor  ->  $v0 = cuántos de sus 8 bits bajos valen 1
#    (revise el bit 0 con andi, desplace con srl, repita 8 veces).
#    Pruébela primero con 0xB5 (1011 0101) y guarde el resultado en $s0.
# 2. Ciclo de sondeo de 20 vueltas ($s7 = vueltas que faltan): en cada vuelta
#    lea los botones, cuéntelos con CONTAR (resultado en $s1) y dibuje en la
#    fila 15 (desde 0x5E0) una barra: los primeros $s1 píxeles en verde (10),
#    el resto de los 8 en negro (0).
#
# Pruébelo en tiempo real: a 10 y presione las teclas 0-7 mientras corre.
# Preguntas: ¿por qué hay que leer 0x600 en CADA vuelta? ¿Qué pasa si activa y
# desactiva un botón muy rápido? ¿Qué ventaja tendría una interrupción?
#
# Complete ej2_botones.esperado (sin botones activados) y verifique con:
#   ./sim -t programas/etapa_4/ej2_botones.asm
.text
# escriba aquí su código

