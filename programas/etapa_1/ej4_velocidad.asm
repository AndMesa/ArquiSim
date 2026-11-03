# Ejercicio 4 -- De m/s a km/h sin multiplicar ni dividir (punto fijo)
# Instrucciones: sll, add, srl
#
#   km/h = 3.6 * (m/s), pero el procesador no tiene decimales.
#   Aproximamos 3.6 con una fracción cuyo denominador es potencia de 2:
#       3.6  ~  461 / 128 = 3.6016
#       km/h ~  (461 * v) >> 7         con  461 = 256 + 128 + 64 + 8 + 4 + 1
#
# Con v = 20 m/s en $s0:
#   $t0 = 461 * v   (solo sll y add)
#   $s1 = $t0 >> 7  (la velocidad en km/h)
#
# Preguntas: pruebe luego con v = 3 (exacto: 10.8 km/h). ¿Qué da el programa y por qué?
# ¿Qué fracción usaría para más precisión y qué riesgo hay si el numerador crece?
#
# Complete ej4_velocidad.esperado y verifique con:
#   ./sim -t programas/etapa_1/ej4_velocidad.asm
.text
        addi $s0, $zero, 20         # v = 20 m/s
# escriba aquí su código

