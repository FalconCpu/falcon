# Overall goal

In this direcotory I am working on a compiler for a language called fpl. 

My goal here is to have a language in which I can write an operating system for my computer - something that can work close to the metal whilst still giving as modern and safe a feel as possible.

There is always that tension in there - On one hand when the computer boots I want just a small amount of assembly code to setup the stack pointer etc - then jump into fpl code. So fpl needs to be able to handle setting up the memory system, working with peripherals etc. Viewing the computer as a big array of bytes and a set of instructions that can manipulate those bytes.

Yet on the other hand I want to make it as ergonomic and safe as possible - given those constraints. 




# Symbols

Used to represent named objects in the compiler frontend. `AstBlock` classes contain a map of names to `Symbol`s declared within them.

In the backend the definition of a symbol widens considerably to represent the result of any operation.

In the front end only the following should appear:-

- `Symbol`
  - `VariableSymbol`      A local or global variable
  - `FunctionSymbol`      Maps a name to a `AstFunction`
  - `TypeSymbol`          Maps a name to a Type
  - `FieldSymbol`         Represent the fields in a class

Additionaly in the backend the following Symbol classes are used:

- `Symbol`
  - `RegisterSymbol`      A CPU register
  - `TempSymbol`          The result of an operation
    - `IntegerSymbol`     A TempSymbol where the result is known to be a constant integer
  - `StringLitSymbol`     A reference to an entry in a global strings pool


In the Backend, up till register allocation, `TempSymbols` should always be SSA. Currently I don't enforce SSA properties on other symbol types.

The follwing properies should always be obeyed:

* All `TempSymbols` are written to by exactly one instruction
* `IntegerSymbol` are only written to by `InstrLdi` instructions (note the converse is not always true - it is possible for `InstrLdi` to write to other `TempSymbol`s)
* `VariableSymbols` are only directly written by `InstrMov` or `InstrLdi` instructions. Results of calculations are always evaluated in a `TempSymbol` then transfered to a `VariableSymbol`
* `FieldSymbol` only appears as the offset or a load or store instruction. Never as the source or destination of other instructions
* `TypeSymbol` , `FunctionSymbol` and `StringLitSymbols`should only appear on the rhs of an `InstrLea`, never in other instructions.