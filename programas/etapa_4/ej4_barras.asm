# Ejercicio 4 -- Gráfico de barras de velocidades
# Instrucciones: las de las Etapas 1 a 3
#
# V tiene 8 velocidades (en 0x200). Dibuje una barra por cada una en la pantalla
# de 32 x 16:
#   - la barra i ocupa las columnas x = 4i+1, 4i+2 y 4i+3 (3 píxeles de ancho)
#   - mide V[i] píxeles de alto y "sale" desde abajo: filas 16 - V[i] hasta 15
#   - color: verde brillante (10) si V[i] < 10, rojo brillante (9) si V[i] >= 10
#
# Sugerencia: un ciclo para las barras y, dentro, otro para las filas.
#
# Preguntas: ¿qué dirección tiene la esquina inferior izquierda de la barra 0?
# ¿Qué pasaría con una velocidad de 20? ¿Cómo la protegería (saturar en 16)?
#
# Complete ej4_barras.esperado y verifique con:
#   ./sim -t programas/etapa_4/ej4_barras.asm
.data
V:      .word 3, 8, 14, 6, 11, 16, 1, 10
.text
# escriba aquí su código

