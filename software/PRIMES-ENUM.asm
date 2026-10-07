; ============================================================
;  PRIMES ENUMERATOR  -  Busch Microtronic 2090
; ============================================================
;  Displays the prime numbers 2, 3, 5, 7, 11, ... one after
;  another on the 6-digit display (in decimal), pausing after
;  each one, then halts after the last prime below 1000 (997).
;
;  Method: trial division using the built-in DIV (F0C).
;    DIV divides regEx[0..3] (dividend) by reg[0..3] (divisor),
;    leaving quotient in reg[0..3], remainder in regEx[0..3],
;    and setting Z = (remainder > 0).  So after DIV:
;        BRZ  -> divisor does NOT divide n   (remainder != 0)
;        fall -> divisor divides n           (remainder == 0)
;    The quotient q gives a free sqrt(n) bound: once q <= d we
;    have tested every divisor up to sqrt(n) and n is prime.
;
;  Registers:
;    R8 = units, R9 = tens, RA = hundreds   -> current n (BCD)
;    R6 = d units, R7 = d tens              -> trial divisor d
;    R0..R3   scratch (DIV divisor / quotient)
;    regEx0..3 dividend / remainder (loaded via MAS)
;    RC,RD,RE delay-loop counters
;    R4,R5    kept 0 (DIV requires all digit regs <= 9)
; ============================================================

; ---------- init: n = 2, show it ----------
start:
    CLEAR                ; all reg[] = 0
    MAS  4               ; regEx4 = 0  (DIV also checks regEx4/5 <= 9)
    MAS  5               ; regEx5 = 0
    MOVI 2,8             ; n = 002
    CALL show            ; display 2 + delay
    MOVI 3,8             ; n = 003, fall into the test loop

; ---------- test current n for primality ----------
test:
    MOVI 2,6             ; d = 02  (start trial divisor at 2)
    MOVI 0,7
trydiv:
    ; dividend n -> regEx[0..3]
    MOV  8,0             ; R0 = n units
    MOV  9,1             ; R1 = n tens
    MOV  A,2             ; R2 = n hundreds
    MOVI 0,3             ; R3 = 0
    MAS  0               ; regEx0 = R0
    MAS  1               ; regEx1 = R1
    MAS  2               ; regEx2 = R2
    MAS  3               ; regEx3 = 0
    ; divisor d -> reg[0..3]
    MOV  6,0             ; R0 = d units
    MOV  7,1             ; R1 = d tens
    MOVI 0,2             ; R2 = 0
    MOVI 0,3             ; R3 = 0
    DIV                  ; R0..3 = q,  regEx0..3 = r,  Z = (r>0)
    BRZ  notdiv          ; r != 0  -> d does not divide n
    GOTO composite       ; r == 0  -> n is composite
notdiv:
    ; sqrt bound: is quotient q <= d ?   (q in R2:R1:R0, d in R7:R6)
    CMPI 0,2             ; q hundreds == 0 ?
    BRZ  qsmall          ;   yes -> q < 100, safe to compare 2 digits
    GOTO incd            ;   no  -> q >= 100 > d, keep dividing
qsmall:
    CMP  1,7             ; q tens  vs d tens :  C=(qT<dT)  Z=(qT==dT)
    BRC  prime           ; q tens < d tens          -> q < d -> prime
    BRZ  cmplo           ; q tens = d tens          -> compare units
    GOTO incd            ; q tens > d tens          -> q > d -> keep dividing
cmplo:
    CMP  6,0             ; d units vs q units :  C=(dU<qU)
    BRC  incd            ; d units < q units -> q > d -> keep dividing
    GOTO prime           ; else q <= d -> prime
incd:
    ADDI 1,6             ; d units ++
    CMPI A,6             ; == 10 ?
    BRZ  incd_c
    GOTO trydiv
incd_c:
    MOVI 0,6             ; d units = 0
    ADDI 1,7             ; d tens ++
    GOTO trydiv

prime:
    CALL show            ; display n + delay, then advance n

; ---------- advance n (BCD +1); halt at 1000 ----------
composite:
    ADDI 1,8             ; units ++
    CMPI A,8
    BRZ  nc1
    GOTO test
nc1:
    MOVI 0,8
    ADDI 1,9             ; tens ++
    CMPI A,9
    BRZ  nc2
    GOTO test
nc2:
    MOVI 0,9
    ADDI 1,A             ; hundreds ++
    CMPI A,A             ; reached 1000 ?
    BRZ  done
    GOTO test
done:
    HALT

; ---------- subroutine: display n, then delay ----------
show:
    DISP 3,8             ; show hundreds:tens:units of n
    MOVI 3,C             ; delay strength (about 1 s per unit); tune 0..F
dOut:
    MOVI F,D             ; one pass of the middle loop only: about 1 s per unit on the real machine
dMid:
    MOVI 0,E
dIn:
    ADDI 1,E
    BRC  dMidDone        ; E wrapped 15->0  (16 iterations)
    GOTO dIn
dMidDone:
    ADDI 1,D
    BRC  dOutStep        ; D wrapped (256 iterations)
    GOTO dMid
dOutStep:
    SUBI 1,C
    BRC  dDone           ; C went below 0 -> finished
    GOTO dOut
dDone:
    RET
