// Headless export: function table, decompiled C and disassembly for one program.
// Usage: analyzeHeadless ... -postScript ExportAll.java <outdir>
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class ExportAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File out = new File(args[0], currentProgram.getName());
        File dc = new File(out, "decomp");
        dc.mkdirs();

        DecompInterface ifc = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        ifc.setOptions(opts);
        ifc.openProgram(currentProgram);

        Listing listing = currentProgram.getListing();
        PrintWriter tsv = new PrintWriter(new File(out, "functions.tsv"));
        tsv.println("addr\tend\tsize\tname\tcc\tsignature\tcallers\tcallees");
        PrintWriter all = new PrintWriter(new File(out, "all.c"));
        PrintWriter asm = new PrintWriter(new File(out, "all.asm"));

        FunctionIterator it = listing.getFunctions(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Function f = it.next();
            if (f.isExternal()) continue;
            AddressSetView body = f.getBody();
            long size = body.getNumAddresses();
            StringBuilder callers = new StringBuilder();
            for (Function c : f.getCallingFunctions(monitor)) callers.append(c.getEntryPoint()).append(',');
            StringBuilder callees = new StringBuilder();
            for (Function c : f.getCalledFunctions(monitor)) callees.append(c.getName()).append(',');
            tsv.printf("%s\t%s\t%d\t%s\t%s\t%s\t%s\t%s%n", f.getEntryPoint(), body.getMaxAddress(), size,
                f.getName(), f.getCallingConventionName(),
                f.getPrototypeString(false, false).replace('\t', ' '), callers, callees);

            asm.printf("%n; ==== %s %s (%d bytes)%n", f.getEntryPoint(), f.getName(), size);
            InstructionIterator ii = listing.getInstructions(body, true);
            while (ii.hasNext()) {
                Instruction ins = ii.next();
                String lbl = "";
                Symbol s = currentProgram.getSymbolTable().getPrimarySymbol(ins.getAddress());
                if (s != null && !ins.getAddress().equals(f.getEntryPoint())) lbl = s.getName() + ":";
                if (!lbl.isEmpty()) asm.println(lbl);
                asm.printf("  %s  %s%n", ins.getAddress(), ins.toString());
            }

            DecompileResults r = ifc.decompileFunction(f, 120, monitor);
            String code = (r != null && r.decompileCompleted()) ? r.getDecompiledFunction().getC()
                : "/* decompile failed: " + (r == null ? "null" : r.getErrorMessage()) + " */\n";
            String hdr = "// " + f.getEntryPoint() + " " + f.getName() + " size=" + size + "\n";
            try (PrintWriter w = new PrintWriter(new File(dc, f.getEntryPoint() + ".c"))) {
                w.print(hdr);
                w.print(code);
            }
            all.print(hdr);
            all.print(code);
            all.println();
        }
        tsv.close();
        all.close();
        asm.close();

        // Exports, imports, defined strings, memory blocks.
        PrintWriter misc = new PrintWriter(new File(out, "symbols.txt"));
        SymbolTable st = currentProgram.getSymbolTable();
        misc.println("# memory blocks");
        for (var b : currentProgram.getMemory().getBlocks())
            misc.printf("%s %s-%s %s%s%s init=%b%n", b.getName(), b.getStart(), b.getEnd(),
                b.isRead() ? "r" : "-", b.isWrite() ? "w" : "-", b.isExecute() ? "x" : "-", b.isInitialized());
        misc.println("# exports");
        for (Address a : st.getExternalEntryPointIterator()) {
            Symbol s = st.getPrimarySymbol(a);
            misc.printf("%s %s%n", a, s == null ? "?" : s.getName());
        }
        misc.println("# imports");
        for (Symbol s : st.getExternalSymbols()) {
            ExternalLocation el = currentProgram.getExternalManager().getExternalLocation(s);
            misc.printf("%s %s%n", el == null ? "?" : el.getLibraryName(), s.getName());
            for (Reference ref : s.getReferences()) misc.printf("  ref %s%n", ref.getFromAddress());
        }
        misc.println("# strings");
        for (Data d : listing.getDefinedData(true)) {
            if (!d.hasStringValue()) continue;
            Object v = d.getValue();
            misc.printf("%s %s%n", d.getAddress(), v == null ? "" : v.toString().replace("\n", "\\n").replace("\r", "\\r"));
        }
        misc.close();
        println("exported " + currentProgram.getName());
    }
}
