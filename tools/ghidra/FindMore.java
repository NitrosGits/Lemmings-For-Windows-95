// Find functions Ghidra's analysis missed: targets of pointers stored in data
// (vtables, callback tables) and code after MSVC alignment padding in .text gaps.
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.app.util.PseudoInstruction;
import ghidra.program.model.symbol.FlowType;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FindMore extends GhidraScript {
    MemoryBlock text;

    boolean inText(long v) {
        return v >= text.getStart().getOffset() && v <= text.getEnd().getOffset();
    }

    // Length of an MSVC padding instruction at a, or 0.
    int padLen(Address a) throws Exception {
        Memory m = currentProgram.getMemory();
        byte[] b = new byte[7];
        int n = m.getBytes(a, b);
        int[] u = new int[7];
        for (int i = 0; i < 7; i++) u[i] = i < n ? (b[i] & 0xff) : -1;
        if (u[0] == 0x90 || u[0] == 0xcc) return 1;
        if (u[0] == 0x8b && u[1] == 0xff) return 2;
        if (u[0] == 0x8d && u[1] == 0x49 && u[2] == 0) return 3;
        if (u[0] == 0x8d && u[1] == 0x64 && u[2] == 0x24 && u[3] == 0) return 4;
        if (u[0] == 0x05 && u[1] == 0 && u[2] == 0 && u[3] == 0 && u[4] == 0) return 5;
        if (u[0] == 0x8d && u[1] == 0x9b && u[2] == 0 && u[3] == 0 && u[4] == 0 && u[5] == 0) return 6;
        if (u[0] == 0x8d && u[1] == 0xa4 && u[2] == 0x24 && u[3] == 0 && u[4] == 0 && u[5] == 0 && u[6] == 0) return 7;
        return 0;
    }

    boolean free(Address a) {
        Listing l = currentProgram.getListing();
        return l.getFunctionContaining(a) == null && l.getDefinedDataContaining(a) == null
            && l.getInstructionContaining(a) == null;
    }

    boolean make(Address a) {
        Listing l = currentProgram.getListing();
        if (l.getFunctionContaining(a) != null || l.getDefinedDataContaining(a) != null) return false;
        if (l.getInstructionAt(a) == null) {
            if (l.getInstructionContaining(a) != null) return false;
            DisassembleCommand dc = new DisassembleCommand(a, null, true);
            if (!dc.applyTo(currentProgram, monitor)) return false;
            if (l.getInstructionAt(a) == null) return false;
        }
        CreateFunctionCmd cf = new CreateFunctionCmd(a);
        boolean ok = cf.applyTo(currentProgram, monitor);
        if (ok) println("new function " + a);
        return ok;
    }

    // Functions whose body stops at a non-terminal instruction because a bogus
    // function was created in the middle of the next instruction: delete the bogus
    // ones, re-disassemble the real flow and regrow the body.
    int fixTruncated() throws Exception {
        Listing l = currentProgram.getListing();
        List<Function> fs = new ArrayList<>();
        for (Function f : l.getFunctions(true)) fs.add(f);
        PseudoDisassembler pd = new PseudoDisassembler(currentProgram);
        int fixed = 0;
        for (Function f : fs) {
            if (f.isDeleted()) continue;
            Instruction last = l.getInstructionContaining(f.getBody().getMaxAddress());
            if (last == null) continue;
            FlowType ft = last.getFlowType();
            if (!ft.hasFallthrough() || ft.isCall()) continue;
            Address fa = last.getFallThrough();
            if (fa == null || f.getBody().contains(fa)) continue;
            Function g0 = l.getFunctionAt(fa);
            if (g0 != null) {
                // MSVC never falls into the next function: this is one function split in two
                println("merge " + g0.getEntryPoint() + " into " + f.getEntryPoint());
                currentProgram.getFunctionManager().removeFunction(fa);
                CreateFunctionCmd.fixupFunctionBody(currentProgram, f, monitor);
                fixed++;
                continue;
            }
            Set<Address> starts = new HashSet<>();
            Address a = fa;
            Address end = fa;
            for (int i = 0; i < 4096; i++) {
                PseudoInstruction pi = pd.disassemble(a);
                if (pi == null) break;
                starts.add(a);
                end = a.add(pi.getLength() - 1);
                FlowType pft = pi.getFlowType();
                if (!pft.hasFallthrough()) break;
                a = a.add(pi.getLength());
            }
            boolean bogus = false;
            for (Function g : l.getFunctions(new AddressSet(fa, end), true)) {
                if (!starts.contains(g.getEntryPoint())) {
                    println("bogus function " + g.getEntryPoint() + " inside flow of " + f.getEntryPoint());
                    currentProgram.getFunctionManager().removeFunction(g.getEntryPoint());
                    bogus = true;
                }
            }
            if (!bogus && l.getInstructionAt(fa) != null && l.getFunctionContaining(fa) != null) continue;
            if (bogus) clearListing(fa, end);
            new DisassembleCommand(fa, null, true).applyTo(currentProgram, monitor);
            CreateFunctionCmd.fixupFunctionBody(currentProgram, f, monitor);
            fixed++;
        }
        return fixed;
    }

    @Override
    public void run() throws Exception {
        for (int i = 0; i < 5; i++) {
            int n = fixTruncated();
            println("fixTruncated: " + n);
            if (n == 0) break;
        }
        Memory mem = currentProgram.getMemory();
        text = mem.getBlock(".text");
        int total = 0;
        for (int pass = 0; pass < 10; pass++) {
            int made = 0;
            // 2. gaps in .text after padding, at 16-byte boundaries
            Address a = text.getStart();
            while (a.compareTo(text.getEnd()) < 0) {
                if ((a.getOffset() & 15) == 0 && getFunctionContaining(a) == null
                        && getDataContaining(a) == null && getInstructionAt(a) != null && make(a)) {
                    made++;
                    continue;
                }
                if (getFunctionContaining(a) == null && getDataContaining(a) == null
                        && getInstructionAt(a) != null) {
                    int p0 = padLen(a);
                    a = a.add(p0 > 0 ? p0 : getInstructionAt(a).getLength());
                    continue;
                }
                if (!free(a)) {
                    Function f = getFunctionContaining(a);
                    Data d = getDataContaining(a);
                    Instruction ins = getInstructionContaining(a);
                    Address next = f != null ? f.getBody().getMaxAddress().add(1)
                        : d != null ? d.getMaxAddress().add(1) : ins.getMaxAddress().add(1);
                    if (f != null) {
                        // body may be fragmented; advance to the next byte past the containing range
                        AddressRange r = f.getBody().getRangeContaining(a);
                        next = r.getMaxAddress().add(1);
                    }
                    a = next;
                    continue;
                }
                int p = padLen(a);
                if (p > 0) { a = a.add(p); continue; }
                if ((a.getOffset() & 15) == 0 && make(a)) { made++; continue; }
                a = a.add(1);
            }
            total += made;
            println("pass " + pass + ": " + made + " new");
            if (made == 0) break;
        }
        for (int i = 0; i < 5; i++) {
            int n = fixTruncated();
            println("fixTruncated after: " + n);
            if (n == 0) break;
        }
        println("FindMore total " + total);
    }
}
