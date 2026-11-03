# Ejercicio 4 -- Velocidad: dividir sin instrucción de división
# Instrucciones: todas las de las Etapas 1 a 3 (incluida la pila)
#
# ArquiSim todavía no tiene div: se divide restando una y otra vez.
#
# 1. Función DIVIDIR: $a0 = dividendo, $a1 = divisor (ambos > 0)
#        $v0 = cociente, $v1 = residuo
#    (mientras dividendo >= divisor: restar el divisor y sumar 1 al cociente)
# 2. Función KMH: $a0 = distancia en metros, $a1 = tiempo en segundos
#        $v0 = velocidad en km/h = ((d / t) * 36) / 10
#    KMH llama a DIVIDIR dos veces: como jal cambia $ra, KMH debe guardar su
#    propio $ra en la pila (push) y recuperarlo antes de volver (pop).
#    (* 36 = * 32 + * 4, con sll)
# 3. Principal: d = 450 m, t = 20 s. Guarde la velocidad en $s0.
#
# Preguntas: 450 m en 20 s es 22.5 m/s = 81 km/h exactos. ¿Qué da su programa y
# por qué? ¿Cómo reordenaría las operaciones para perder menos precisión?
# ¿Qué pasa si KMH no guarda $ra?
#
# Complete ej4_velocidad.esperado y verifique con:
#   ./sim -t programas/etapa_3/ej4_velocidad.asm
.text
# escriba aquí su código

