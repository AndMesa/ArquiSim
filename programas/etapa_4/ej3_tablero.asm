# Ejercicio 3 -- Un tablero de ajedrez en la pantalla RGB (32 x 32, 24 bits)
# Instrucciones: las de las Etapas 1 a 3
#
#   0x604 POSICION  (y << 16) | x
#   0x608 COLOR     0x00RRGGBB: escribirlo dibuja el píxel y AVANZA x solo
#
# Dibuje casillas de 4 x 4 píxeles: rojo (0xFF0000) y azul (0x0000FF).
# El píxel (x, y) es ROJO si el bit 2 de (x XOR y) vale 1, y AZUL si vale 0.
# Escriba la posición UNA sola vez y luego los 1024 colores seguidos.
#
# Preguntas: ¿por qué (x XOR y) AND 4 produce casillas de 4 x 4? ¿Cómo haría
# casillas de 8 x 8? ¿Cuánto vale 0x604 al final y por qué? ¿Cuántos sw hizo, y
# cuántos haría si no existiera el autoincremento?
#
# Complete ej3_tablero.esperado y verifique con:
#   ./sim -t programas/etapa_4/ej3_tablero.asm
.text
# escriba aquí su código

