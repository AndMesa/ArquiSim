# Programa 03 -- Load/Store:  A[12] = h + A[2]
# Base de A -> $s3, h -> $s2
# Necesita: lw, sw
#
# Recuerde: cada palabra ocupa 4 bytes, así que A[i] está en base + 4*i.

.data
A:      .word 0, 0, 100             # A[0], A[1], A[2] = 100
        .space 40                   # A[3] .. A[12]

.text
        addi $s3, $zero, A          # $s3 = dirección base de A (0x200)
        addi $s2, $zero, 4          # h = 4
        lw   $t0, 8($s3)            # $t0 = A[2]          (2 x 4 = 8)
        add  $t0, $s2, $t0          # $t0 = h + A[2] = 104
        sw   $t0, 48($s3)           # A[12] = $t0         (12 x 4 = 48)
