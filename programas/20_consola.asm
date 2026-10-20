# Programa 20 -- Consola: servicios del sistema (syscall)
#
# El número de servicio va en $v0 y el argumento en $a0:
#   1 imprimir entero   4 imprimir cadena   5 leer entero (-> $v0)
#   10 terminar         11 imprimir carácter
# Las cadenas se declaran en .data con .asciiz (terminan en un byte 0).
# Necesita: syscall, mul + las pseudoinstrucciones li, la, move

.data
PIDE:   .asciiz "Ingrese un número (0 para terminar): "
RES:    .asciiz "  su cuadrado es "
ADIOS:  .asciiz "Suma de los cuadrados: "

.text
LEER:   li   $v0, 4                 # imprimir cadena
        la   $a0, PIDE
        syscall
        li   $v0, 5                 # leer entero -> $v0
        syscall
        beq  $v0, $zero, SALIR
        move $t0, $v0
        mul  $t1, $t0, $t0          # cuadrado
        add  $s0, $s0, $t1          # suma de cuadrados
        li   $v0, 4
        la   $a0, RES
        syscall
        li   $v0, 1                 # imprimir entero
        move $a0, $t1
        syscall
        li   $v0, 11                # imprimir carácter: salto de línea
        li   $a0, 10
        syscall
        j    LEER

SALIR:  li   $v0, 4
        la   $a0, ADIOS
        syscall
        li   $v0, 1
        move $a0, $s0
        syscall
        li   $v0, 10                # terminar el programa
        syscall
        addi $s7, $zero, 1          # nunca se ejecuta
