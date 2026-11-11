# Ejercicio 3 -- Velocidad con mult y div (registros HI y LO)
# Instrucciones: div, mult, mul, mfhi, mflo (+ las anteriores)
#
# d = 450 m, t = 20 s (como en el Ejercicio 3.4, que dividía restando).
#
# 1. $s1 = d / t en m/s y $s2 = el residuo   (div deja LO = cociente, HI = residuo)
# 2. $s0 = velocidad en km/h, pero ahora SIN perder precisión: primero multiplique
#    y al final divida:   km/h = (d * 36) / (t * 10)
#
# Preguntas: ¿qué da ahora y qué daba el Ejercicio 3.4 (79)? ¿Por qué el orden
# de las operaciones cambia el resultado? ¿Cuántas instrucciones ejecuta este
# programa comparado con el 3.4? ¿Qué pasa si t = 0?
#
# Complete ej3_velocidad.esperado y verifique con:
#   ./sim -t programas/etapa_5/ej3_velocidad.asm
.text
        li   $s3, 450               # d (metros)
        li   $s4, 20                # t (segundos)
# escriba aquí su código

