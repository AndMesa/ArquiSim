# Programa 19 -- Multiplicación, división (registros HI y LO) y saltos de comparación
#
# mult/div dejan su resultado en HI y LO (fuera del banco de registros):
#   mult: HI:LO = producto de 64 bits     div: LO = cociente, HI = residuo
# blt/bgt/ble/bge son pseudoinstrucciones: el ensamblador las expande con $at.
# Necesita: div, mult, mfhi, mflo, mul, srl, andi, slt, beq, bne, lui, ori, nor
#           + las pseudoinstrucciones li, move, not, blt, ble

.data
COLOR:  .word 0x00C86432            # R = 200, G = 100, B = 50

.text
# 1) Gris = (R + G + B) / 3, como en el Proyecto 2
        lw   $t0, COLOR($zero)
        andi $t1, $t0, 0xFF         # B = 50
        srl  $t0, $t0, 8
        andi $t2, $t0, 0xFF         # G = 100
        srl  $t0, $t0, 8
        andi $t3, $t0, 0xFF         # R = 200
        add  $t4, $t1, $t2
        add  $t4, $t4, $t3          # R + G + B = 350
        li   $t5, 3
        div  $t4, $t5               # LO = 116, HI = 2
        mflo $s0                    # gris = 116
        mfhi $s1                    # residuo = 2

# 2) Un producto que no cabe en 32 bits: 100000 * 100000 = 10^10
        li   $t0, 100000            # (no cabe en 16 bits: lui + ori)
        mult $t0, $t0               # HI:LO = 0x00000002_540BE400
        mfhi $s2
        mflo $s3
        mul  $s4, $t5, $t5          # mul: solo los 32 bits bajos (3 * 3 = 9)

# 3) Saturación: brillo = gris + 150, sin pasar de 255
        addi $t6, $s0, 150          # 266
        ble  $t6, 255, LISTO        # ble con un número: addi $at + slt + beq
        li   $t6, 255
LISTO:  move $s5, $t6               # 255

# 4) Ciclo con blt: suma 0 + 1 + ... + 9
        add  $s6, $zero, $zero
        add  $t7, $zero, $zero
CICLO:  add  $s6, $s6, $t7
        addi $t7, $t7, 1
        blt  $t7, 10, CICLO         # 45
        not  $s7, $zero             # 0xFFFFFFFF
