# Programa 17 -- Interrupciones: botones y temporizador
#
# El programa principal NO lee los botones: solo cuenta en un ciclo infinito.
# Cuando un botón se activa o se cumple el periodo del temporizador, el hardware
# interrumpe, guarda el PC en EPC y salta a la ISR. La ISR atiende la causa y
# vuelve con eret al punto exacto donde iba el programa principal.
#
#   0x604 TEMPORIZADOR  interrumpir cada N instrucciones
#   0x608 VECTOR        dirección de la ISR
#   0x60C CAUSA         bit 0 = botones, bit 1 = temporizador
#
# Pruébelo en tiempo real:  a 20  y presione las teclas 0-7 mientras corre.
# Necesita: ei, eret, lw, sw, sb, andi, srl, beq, j
# (la ISR usa $k0, $k1 -reservados para el sistema- y registros $t que el programa
#  principal no usa; una ISR real los guardaría en la pila)

.text
main:   addi $t0, $zero, ISR
        sw   $t0, 0x608($zero)      # VECTOR = dirección de la ISR
        addi $t0, $zero, 40
        sw   $t0, 0x604($zero)      # TEMPORIZADOR: interrumpir cada 40 instrucciones
        addi $s0, $zero, 0x400      # base de la pantalla
        ei                          # habilitar interrupciones

IDLE:   addi $s7, $s7, 1            # programa principal: solo cuenta
        j    IDLE

# ---------------- Rutina de servicio de interrupción ----------------
ISR:    lw   $k0, 0x60C($zero)      # CAUSA

        andi $k1, $k0, 2            # ¿temporizador?
        beq  $k1, $zero, ISR_BOT
        addi $s1, $s1, 1            # ticks del reloj
        andi $t0, $s1, 31           # x = ticks mod 32
        addi $t0, $t0, 480          # fila 15 = 15 * 32
        add  $t0, $s0, $t0
        addi $t1, $zero, 12         # azul brillante
        sb   $t1, 0($t0)

ISR_BOT:
        andi $k1, $k0, 1            # ¿botones?
        beq  $k1, $zero, ISR_FIN
        addi $s2, $s2, 1            # interrupciones de botones atendidas
        lw   $t0, 0x600($zero)      # estado de los botones
        addi $t2, $s0, 7            # píxel del botón 0 (x = 7)
        addi $t3, $zero, 8
COPIAR: beq  $t3, $zero, ISR_FIN
        andi $t1, $t0, 1
        beq  $t1, $zero, NEGRO
        addi $t1, $zero, 10         # verde brillante
NEGRO:  sb   $t1, 0($t2)
        srl  $t0, $t0, 1
        addi $t2, $t2, -1
        addi $t3, $t3, -1
        j    COPIAR

ISR_FIN:
        eret                        # PC = EPC, IE = 1
