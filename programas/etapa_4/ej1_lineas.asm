# Ejercicio 1 -- Dibujar líneas en la pantalla de 32 x 16
# Instrucciones: las de las Etapas 1 a 3 (no hay instrucciones nuevas)
#
#   dirección del píxel (x, y) = 0x400 + 32*y + x        (1 byte por píxel)
#   colores: 3 amarillo, 6 cian (ver README para el resto)
#
# 1. Línea HORIZONTAL amarilla en la fila y = 8, de x = 4 a x = 27.
# 2. Después, línea VERTICAL cian en la columna x = 16, de y = 2 a y = 13.
#    (en el cruce queda el color de la última que se dibuje)
#
# Preguntas: ¿qué dirección tiene el píxel del cruce (16, 8)? ¿Por qué la línea
# vertical avanza de 32 en 32 bytes y la horizontal de 1 en 1? ¿Cuántos sb hizo?
#
# Complete ej1_lineas.esperado y verifique con:
#   ./sim -t programas/etapa_4/ej1_lineas.asm
.text
# escriba aquí su código

