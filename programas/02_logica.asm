# Programa 02 -- Lógica, máscaras y desplazamientos
# Necesita: lui, ori, and, andi, nor, sll, srl, sra

.text
        lui  $s1, 0x8A2C            # $s1 = 0x8A2C0000
        ori  $s1, $s1, 0x9031       # $s1 = 0x8A2C9031  (constante de 32 bits)

        lui  $t0, 0x8000            # máscara del bit más significativo
        and  $t1, $s1, $t0          # $t1 = 0x80000000  -> el MSB es 1
        ori  $t2, $s1, 0x00FF       # $t2 = 0x8A2C90FF  -> encender los 8 bits bajos
        andi $t3, $s1, 0x000F       # $t3 = 0x00000001  -> aislar los 4 bits bajos

        sll  $t4, $t3, 4            # $t4 = 0x10  (multiplicar por 16)
        srl  $t5, $s1, 28           # $t5 = 0x8   (rellena con ceros)
        sra  $t6, $s1, 28           # $t6 = 0xFFFFFFF8 (conserva el signo)
        nor  $t7, $zero, $zero      # $t7 = 0xFFFFFFFF (NOT 0)
