# Ejercicio 3 -- Una función: el mayor de tres números
# Instrucciones: jal, jr (+ las de las Etapas 1 a 3)
#
#   int max3(int x, int y, int z);      // argumentos en $a0, $a1, $a2; retorno en $v0
#   s0 = max3(4, 9, 2);
#   s1 = max3(-1, -7, -3);
#
# 1. Escriba la función MAX3 (termina con jr $ra).
# 2. En el programa principal, llámela dos veces con jal y guarde los resultados.
# 3. Después de las llamadas, salte al final (j FIN) para no volver a entrar a MAX3.
#
# Preguntas: ¿qué valor tiene $ra después de cada jal y por qué es distinto?
# ¿Qué pasaría si olvida el "j FIN"?
#
# Complete ej3_funcion.esperado y verifique con:
#   ./sim -t programas/etapa_3/ej3_funcion.asm
.text
# escriba aquí su código

