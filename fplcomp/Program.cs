
using System.Runtime.CompilerServices;

class Program {
    static List<string> files = [];
    static string outputFilename = "output.f32";
    static Boolean includeStdlib = true;
    static Boolean includeOs = true;
    static Boolean verbose = false;

    static AstTop topLevel = new();

    const int PHASE_PARSE = 1;
    const int PHASE_TYPECHECK = 2;
    const int PHASE_CODEGEN = 3;
    const int PHASE_BACKEND = 4;
    const int PHASE_ALL = 5;
    static int stopAt = PHASE_ALL;

    readonly static string   stdLibPath = "C:\\Users\\simon\\falcon\\fplcomp\\stdlib";
    readonly static string   osLibPath   = "C:\\Users\\simon\\falcon\\os\\src";

    static void parseArgs(string[] args) {
        if (args.Length==0) {
            Log.Error("Usage: fplcomp <options> <files>");
            return;
        }

        var index =0; 
        while(index<args.Length) {
            string arg = args[index++];
            switch(arg) {
                case "-stop_at_parse":
                    stopAt = PHASE_PARSE;
                    break;

                case "-stop_at_typecheck":
                    stopAt = PHASE_TYPECHECK;
                    break;

                case "-stop_at_codegen":
                    stopAt = PHASE_CODEGEN;
                    break;

                case "-stop_at_backend":
                    stopAt = PHASE_BACKEND;
                    break;

                case "-o":
                    outputFilename = args[index++];
                    break;

                case "-no_stdlib":
                    includeStdlib = false;
                    break;

                case "-no_os":
                    includeOs = false;
                    break;

                case "-v":
                    verbose = true;
                    break;

                default:
                    if (arg.StartsWith('-'))
                        Console.Out.WriteLine($"Unknown option: {arg}");
                    else
                        files.Add(arg);
                    break;
            }
        }
    }

    static List<string> GetFilesToCompile() {
        List<string> ret = [];

        // Get the memory allocator from OS if we are compiling for OS, else get it from the stdlib
        if (includeOs)
            ret.Add($"{osLibPath}\\Memory.fpl");
        else if (includeStdlib)
            ret.Add($"{stdLibPath}\\Memory.fpl");

        if (includeOs) {
            // Include the files from the OS
            string lstFileName = $"{osLibPath}\\files.lst";
            string[] filesList = File.ReadAllLines(lstFileName);
            foreach (string file in filesList) {
                if (!string.IsNullOrWhiteSpace(file)) {
                    ret.Add($"{osLibPath}\\{file.Trim()}");
                }
            }
        }

        if (includeStdlib) {
            // Include the files from the OS
            string lstFileName = $"{stdLibPath}\\files.lst";
            string[] filesList = File.ReadAllLines(lstFileName);
            foreach (string file in filesList) {
                if (!string.IsNullOrWhiteSpace(file)) {
                    ret.Add($"{stdLibPath}\\{file.Trim()}");
                }
            }
        }

        ret.AddRange(files);
        return ret;
    }

    static void RunParser() {
 
        foreach (string file in GetFilesToCompile()) {
            try {
                if (verbose)
                    Console.Out.WriteLine($"Parsing {file}");
                Lexer lexer = new(file);
                Parser parser = new(lexer);
                parser.Parse(topLevel);
            } catch (FileNotFoundException e) {
                Log.Error($"File not found: {e.FileName}");
            }
        }
    }

    static void GenerateAssembly() {
        OutputAssembly output = new( new StreamWriter(outputFilename));
        foreach(AstFunction func in AstFunction.allFunctions)
            output.Output(func);
        foreach(ConstObjectSymbol cos in ConstObjectSymbol.allConstObjects)
            output.Output(cos);
        output.Close();
    }

    static int Main(string[] args) {
        

        // Parse the arguments
        parseArgs(args);

        // Run the compilation
        for(int phase=1; phase<stopAt; phase++) {
            switch(phase) {
                case PHASE_PARSE: 
                    RunParser();  
                    break;

                case PHASE_TYPECHECK: 
                    topLevel.TypeCheck(topLevel, new PathContext());
                    break;

                case PHASE_CODEGEN: 
                    foreach(AstFunction func in AstFunction.allFunctions)
                        func.CodeGen(func);
                    break;

                case PHASE_BACKEND: 
                    foreach(AstFunction func in AstFunction.allFunctions)
                        func.RunBackend();
                    break;

                default:
                    Log.Error($"Unknown phase {phase}");
                    break;
            }

            if (Log.numErrors!=0) {
                Console.WriteLine($"Failed with {Log.numErrors} errors");
                return Log.numErrors;
            }
        }

        // Output the result for the phase we stopped at
        switch(stopAt) {
            case PHASE_PARSE:     topLevel.Print(0);    break;
            case PHASE_TYPECHECK: topLevel.Print(0);    break;
            case PHASE_CODEGEN:   topLevel.PrintCode(); break;
            case PHASE_BACKEND:   topLevel.PrintCode(); break;
            case PHASE_ALL:       GenerateAssembly();   break;
            default: Log.Error($"Unknown phase {stopAt}"); break;
        }

        return 0;
    }
}
