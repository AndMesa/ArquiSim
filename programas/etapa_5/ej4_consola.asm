# Ejercicio 4 -- Calculadora de velocidad en la consola (syscall)
# Instrucciones: syscall (+ todas las anteriores)
#
#   $v0 = servicio:  1 imprimir entero ($a0)   4 imprimir cadena ($a0 = dirección)
#                    5 leer entero (-> $v0)    10 terminar   11 imprimir carácter
#
# Repita:
#   - pida la distancia en metros (0 = terminar) y el tiempo en segundos
#   - imprima la velocidad en km/h = (d * 36) / (t * 10)
#   - cuente los viajes en $s1 y deje la última velocidad en $s0
# Al terminar imprima "Viajes: n" y finalice con el servicio 10.
#
# Ejecútelo en la interfaz: la vista CONSOLA aparece sola; escriba los números
# en la línea de comandos. En la prueba (-t) los números se leen de
# ej4_consola.entrada: 450, 20, 1000, 50, 0.
#
# Preguntas: ¿qué pasa si el tiempo es 0? Agregue una validación que imprima un
# mensaje y vuelva a pedir el tiempo. ¿Por qué syscall necesita un número de
# servicio en $v0 y no una instrucción distinta para cada servicio?
#
# Complete ej4_consola.esperado y verifique con:
#   ./sim -t programas/etapa_5/ej4_consola.asm
.data
PIDE_D: .asciiz "Distancia en metros (0 = salir): "
PIDE_T: .asciiz "Tiempo en segundos: "
VEL:    .asciiz "  velocidad: "
KMH:    .asciiz " km/h\n"
VIAJES: .asciiz "Viajes: "
.text
# escriba aquí su código

