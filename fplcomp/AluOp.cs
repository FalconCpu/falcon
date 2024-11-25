public enum  AluOp {
    UNDEFINED,
    ADD_I,
    SUB_I,
    MUL_I,
    DIV_I,
    MOD_I,
    AND_I,
    OR_I,
    XOR_I,
    LSL_I,
    LSR_I,
    ASR_I,
    EQ_I,
    NE_I,
    LT_I,
    LE_I,
    GT_I,
    GE_I,

    ADD_F,
    SUB_F,
    MUL_F,
    DIV_F,
    EQ_F,
    NE_F,
    LT_F,
    LE_F,
    GT_F,
    GE_F,

    ADD_S,
    EQ_S,
    NE_S,
    LT_S,
    LE_S,
    GT_S,
    GE_S,

    AND_B,
    OR_B
}

public static class AluOpExtensions {
    public static bool IsCommutative(this AluOp op) {
        return op switch
        {
            AluOp.ADD_I or AluOp.AND_I or AluOp.OR_I or AluOp.XOR_I or AluOp.MUL_I or AluOp.ADD_F or 
            AluOp.MUL_F or AluOp.EQ_I or AluOp.NE_I or AluOp.EQ_F or AluOp.NE_F or AluOp.EQ_S or AluOp.NE_S => true,
            _ => false,
        };
    }
}