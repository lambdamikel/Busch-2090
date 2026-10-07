; ============================================================
; PRIMES-LOOP  -  endless prime-number enumerator for the Busch Microtronic 2090
;
; Shows the primes 2, 3, 5, 7, ... 997 and then starts again at 2.
;   * At the start it waits for ONE key: the pause after each prime, 0..F (about 1 s per unit).
;   * The display stays on the last prime found while the next one is searched
;     (it shows registers C,D,E, which only change when a new prime is latched).
;   * The output LEDs show the trial divisor's last digit while it works.
;
; Registers:  0-3 DIV scratch   4,5 kept 0 (DIV needs them 0; 4 doubles as the pause counter and ends at 0)
;             6,7 trial divisor d (units, tens)   8,9,A candidate n (units, tens, hundreds)
;             B pause length (keyed in)   C,D,E last prime (shown)   F pause counter
; Method: trial division with the built-in DIV; n is prime once the quotient is <= the divisor.
; ============================================================
start:
    CLEAR
    MAS  4               ; memory registers 4,5 = 0 as well (DIV checks them)
    MAS  5
    KIN  B               ; wait for a key: pause length 0..F
show2:
    DISOUT
    MOVI 2,C             ; last prime := 002
    MOVI 0,D
    MOVI 0,E
    DISP 3,C
    CALL pause
    MOVI 3,8             ; n := 003
    MOVI 0,9
    MOVI 0,A
test:
    MOVI 2,6             ; d := 02
    MOVI 0,7
trydiv:
    DOT  6               ; blinkenlights: the trial divisor
    MOV  8,0             ; dividend n -> memory registers 0..3
    MOV  9,1
    MOV  A,2
    MOVI 0,3
    MAS  0
    MAS  1
    MAS  2
    MAS  3
    MOV  6,0             ; divisor d -> registers 0..3
    MOV  7,1
    MOVI 0,2
    MOVI 0,3
    DIV                  ; quotient in 0..3, remainder in memory registers; Z set = remainder > 0
    BRZ  notdiv
    GOTO composite       ; remainder 0: d divides n
notdiv:
    CMPI 0,2             ; quotient >= 100 ?
    BRZ  qsmall
    GOTO incd            ;   yes: still larger than d
qsmall:
    CMP  1,7             ; quotient tens vs d tens
    BRC  prime           ;   smaller: q < d, so n is prime
    BRZ  cmplo
    GOTO incd
cmplo:
    CMP  6,0             ; d units vs quotient units
    BRC  incd            ;   d < q: keep dividing
    GOTO prime
incd:
    ADDI 1,6             ; d := d + 1 (decimal)
    CMPI A,6
    BRZ  incdc
    GOTO trydiv
incdc:
    MOVI 0,6
    ADDI 1,7
    GOTO trydiv
prime:
    DISOUT               ; blank for an instant, so no half-updated number is ever shown
    MOV  8,C             ; latch n as the last prime
    MOV  9,D
    MOV  A,E
    DISP 3,C
    CALL pause
composite:
    ADDI 1,8             ; n := n + 1 (decimal)
    CMPI A,8
    BRZ  nc1
    GOTO test
nc1:
    MOVI 0,8
    ADDI 1,9
    CMPI A,9
    BRZ  nc2
    GOTO test
nc2:
    MOVI 0,9
    ADDI 1,A
    CMPI A,A             ; reached 1000: start again at 2
    BRZ  show2
    GOTO test
; ---------- pause: B units of about one second ----------
pause:
    MOV  B,F
ploop:
    CMPI 0,F
    BRZ  pdone
pinner:
    ADDI 1,4             ; 16 rounds; register 4 is back at 0 afterwards
    BRC  pnext
    GOTO pinner
pnext:
    SUBI 1,F
    GOTO ploop
pdone:
    RET
