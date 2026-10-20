# Programa 12 -- Pseudoinstrucciones
# El ensamblador las traduce a instrucciones reales: compare la columna de
# código máquina con lo que se escribió. li con 32 bits ocupa DOS instrucciones.
# Necesita: add, addi, sll, beq, lui, ori + las pseudoinstrucciones en isa.txt

.data
A:      .word 7

.text
        li    $t0, 42               # addi $t0, $zero, 42
        li    $t1, 0x12345678       # lui + ori (no cabe en 16 bits)
        la    $s0, A                # addi $s0, $zero, 0x200
        move  $t2, $t0              # add  $t2, $t0, $zero
        nop                         # sll  $zero, $zero, 0
        b     SALTO                 # beq  $zero, $zero, SALTO
        li    $t3, 99               # nunca se ejecuta
SALTO:  clear $t0                   # add  $t0, $zero, $zero
