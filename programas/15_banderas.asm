# Programa 15 -- Registro de banderas Z N C V y saltos según banderas (como en el PDUA)
#
# Las instrucciones aritméticas y lógicas, las del acumulador y cmp actualizan las banderas
# (ojo: una addi entre la comparación y el salto las cambiaría):
#   Z = resultado cero   N = negativo   C = acarreo/préstamo   V = desbordamiento con signo
# Necesita: mtacc, add/sub/addi de 1 dirección, cmp, jz, jn, jv, lui, ori

.text
# 1) Cuenta regresiva con el acumulador: repetir hasta que ACC llegue a 0
        addi  $t0, $zero, 5
        mtacc $t0                   # ACC = 5
        addi  $t1, $zero, 1
        add   $s0, $zero, $zero     # vueltas = 0
CICLO:  addi  $s0, $s0, 1           # vueltas++  (antes de restar: también cambia las banderas)
        sub   $t1                   # ACC = ACC - 1   (Z = 1 cuando llega a 0)
        jz    COMPARAR
        j     CICLO

# 2) cmp calcula rs - rt SOLO para las banderas
COMPARAR:
        addi  $t2, $zero, 3
        addi  $t3, $zero, 7
        cmp   $t2, $t3              # 3 - 7 < 0  ->  N = 1, C = 1 (préstamo)
        jn    MENOR
        j     DESBORDE
MENOR:  addi  $s1, $zero, 1         # s1 = 1: 3 es menor que 7

# 3) Desbordamiento: 0x7FFFFFFF + 1 no detiene el procesador, enciende V
DESBORDE:
        lui   $t4, 0x7FFF
        ori   $t4, $t4, 0xFFFF
        mtacc $t4                   # ACC = el mayor entero positivo
        addi  1                     # ACC = 0x80000000  ->  N = 1, V = 1
        jv    HAY_V
        j     FIN
HAY_V:  addi  $s2, $zero, 1         # s2 = 1: se detectó el desbordamiento
FIN:
