        LOAD 0
        ADD 1
        STORE 2
        SUB 3
        JUMPP POS
        JUMPN NEG
        JUMPZ ZERO
        JUMP END
POS:    LOAD 4
        JUMP CONT
NEG:    LOAD 5
        JUMP CONT
ZERO:   LOAD 6
CONT:   STORE 7
        ADD 8
        SUB 9
END:    HALT
