
class Program {
    static int Main(string[] args) {
        int stopAt = 0;
        string outputFilename = "out.f32";

        if (args.Length==0) {
            // Hack to be able to run debugger
            args = ["c:\\users\\Simon\\falcon\\fplcomp\\example.fpl"];
        }

        AstTop topLevel = new();

        for(int index=0; index<args.Length; index++) {
            switch(args[index]) {
                case "-stop_at_parse": 
                    stopAt = 1;
                    break;

                case "-stop_at_typecheck":
                    stopAt = 2;
                    break;

                case "-stop_at_codegen":
                    stopAt = 3;
                    break;

                case "-stop_at_backend":
                    stopAt = 4;
                    break;

                case "-o":
                    index++;
                    outputFilename = args[index];
                    break;

                default:
                    Lexer lexer = new(args[index]);
                    Parser parser = new(lexer);
                    parser.Parse(topLevel);
                    break;
            }
        }

        if (Log.numErrors!=0) {
            Console.WriteLine($"Failed with {Log.numErrors} errors");
            return Log.numErrors;
        }
            
        if (stopAt==1) {
            topLevel.Print(0);
            return 0;
        }

        topLevel.TypeCheck(topLevel, new PathContext());

        if (Log.numErrors!=0) {
            Console.WriteLine($"Failed with {Log.numErrors} errors");
            return Log.numErrors;
        }
        if (stopAt==2) {
            topLevel.Print(0);
            return 0;
        }

        foreach(AstFunction func in AstFunction.allFunctions)
            func.CodeGen(func);

        if (Log.numErrors!=0) {
            Console.WriteLine($"Failed with {Log.numErrors} errors");
            return Log.numErrors;
        }
        if (stopAt==3) {
            topLevel.PrintCode();
            return 0;
        }

        foreach(AstFunction func in AstFunction.allFunctions)
            func.RunBackend();

        if (Log.numErrors!=0) {
            Console.WriteLine($"Failed with {Log.numErrors} errors");
            return Log.numErrors;
        }
        if (stopAt==4) {
            topLevel.PrintCode();
            return 0;
        }

        OutputAssembly output = new( new StreamWriter("output.f32"));
        foreach(AstFunction func in AstFunction.allFunctions)
            output.Output(func);
        foreach(ConstObjectSymbol cos in ConstObjectSymbol.allConstObjects)
            output.Output(cos);
        output.Close();
        return 0;
    }
}
