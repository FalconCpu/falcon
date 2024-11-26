// Simple peephole optimizer for 

class Peephole {
    private readonly AstFunction func;
    private bool madeChange = false;
    private bool unreachable = false;


    private void ChangeToNop(Instr instr) {
        Debug.PrintLn($"Replacing {instr.index} with NOP");
        func.code[instr.index] = new InstrNop();
        madeChange = true;
    }

    private void ChangeToNop(int index) {
        Debug.PrintLn($"Replacing {index} with NOP");
        func.code[index] = new InstrNop();
        madeChange = true;
    }

    private void ReplaceWith(Instr instr, Instr newInstr) {
        Debug.PrintLn($"Replacing {instr.index} with {newInstr}");
        newInstr.index = instr.index;
        func.code[instr.index] = newInstr;
        madeChange = true;
    }

    private static AluOp InvertBra(AluOp op) {
        return op switch {
            AluOp.EQ_I => AluOp.NE_I,
            AluOp.NE_I => AluOp.EQ_I,
            AluOp.LT_I => AluOp.GE_I,
            AluOp.LE_I => AluOp.GT_I,
            AluOp.GT_I => AluOp.LE_I,
            AluOp.GE_I => AluOp.LT_I,
            _ => throw new ArgumentException("Invalid AluOp"),
        };
    }

    private static bool IsPowerOfTwo(int x) {
        return x != 0 && (x & (x - 1)) == 0;
    }   

    private static int log2(int x) {
        int r = 0;
        while (x > 1) {
            x >>= 1;
            r++;
        }
        return r;
    }

    private void Optimize(Instr instr) {
        // Still to do:
        // Constant Folding: If both operands of an ALU instruction are constants, replace the instruction with an immediate constant assignment.
        // Strength Reduction: Replace expensive operations with cheaper equivalents (e.g., MUL x, 2 → LSL x, 1).
        // Branch-to-Jump Simplification: Convert conditional branches to unconditional jumps if the condition is always true or false.

        // The unreachable flag is set when we see a JMP instruction, and cleared when we see a label.
        if (instr is InstrLabel)
            unreachable = false;
        if (unreachable) {
            ChangeToNop(instr);
            return;
        }

        // If the destination of an instruction is unused, we can remove the instruction.
        if (instr.GetDest() is Symbol sym && sym.useCount==0 && !(instr is InstrCall) && !(instr is InstrCallr)) {
            ChangeToNop(instr);
            return;
        }

        switch (instr) {
            
            case InstrMov mov: {
                if (mov.src == mov.dest)
                    ChangeToNop(mov);

                else if (mov.src is IntegerSymbol isym)
                    ReplaceWith(mov, new InstrLdi(mov.dest, isym.value));
                break;
            }

            case InstrAlu alu: {
                if (alu.rhs is IntegerSymbol isym && isym.value>=-0x1000 && isym.value<0x1000)
                    ReplaceWith(alu, new InstrAlui(alu.dest, alu.op, alu.lhs, isym.value));
                break;
            }

            case InstrAlui alui: {
                if (alui.value==0 && (alui.op==AluOp.ADD_I || alui.op==AluOp.SUB_I || alui.op==AluOp.OR_I || alui.op==AluOp.XOR_I || alui.op==AluOp.LSL_I || alui.op==AluOp.LSR_I))
                    ReplaceWith(alui, new InstrMov(alui.dest, alui.lhs));
                else if (alui.value==0 && (alui.op==AluOp.AND_I || alui.op==AluOp.MUL_I))
                    ReplaceWith(alui, new InstrLdi(alui.dest, 0));
                else if (alui.value==1 && (alui.op==AluOp.MUL_I))
                    ReplaceWith(alui, new InstrMov(alui.dest, alui.lhs));
                else if (IsPowerOfTwo(alui.value) && (alui.op==AluOp.MUL_I))
                    ReplaceWith(alui, new InstrAlui(alui.dest, AluOp.LSL_I, alui.lhs, log2(alui.value)));
                else if (IsPowerOfTwo(alui.value) && (alui.op==AluOp.DIV_I))
                    ReplaceWith(alui, new InstrAlui(alui.dest, AluOp.ASR_I, alui.lhs, log2(alui.value)));
                else if (IsPowerOfTwo(alui.value) && (alui.op==AluOp.MOD_I))
                    ReplaceWith(alui, new InstrAlui(alui.dest, AluOp.AND_I, alui.lhs, alui.value-1) );
                break;
            }


            case InstrBra bra: {
                if (bra.label.index == bra.index+1)
                    ChangeToNop(bra);
                else if (bra.label.index == bra.index+2 && func.code[bra.index+1] is InstrJmp jmp) {
                    ReplaceWith(bra, new InstrBra(InvertBra(bra.op), bra.lhs, bra.rhs, jmp.label));
                    ChangeToNop(jmp);
                }
                else if (bra.lhs is IntegerSymbol isym && isym.value==0)
                    ReplaceWith(bra, new InstrBra(bra.op, RegisterSymbol.registers[0], bra.rhs, bra.label));
                else if (bra.rhs is IntegerSymbol irsym && irsym.value==0)
                    ReplaceWith(bra, new InstrBra(bra.op, bra.lhs, RegisterSymbol.registers[0], bra.label));
                else if (func.code[bra.label.index+1] is InstrJmp jmpb)
                    ReplaceWith(bra, new InstrBra(bra.op, bra.lhs, bra.rhs, jmpb.label));
                break;
            }

            case InstrJmp jmp: {
                if (jmp.label.index == jmp.index+1)
                    ChangeToNop(jmp);
                unreachable = true;
               break;
            }

            case InstrLabel labins: {
                if (labins.label.useCount==0)
                    ChangeToNop(labins);
                break;
            }

        }
    }

    public Peephole(AstFunction func) {
        this.func = func;

        Debug.PrintLn($"Peephole optimization of {func.name}"); 
        func.RebuildIndex();
        do {
            madeChange = false;
            func.PrintCode();
            for(int i=0; i<func.code.Count; i++)
                Optimize(func.code[i]);
            func.RebuildIndex();
        } while (madeChange);

    }
}