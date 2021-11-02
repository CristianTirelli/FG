# FG
Custom Flow Graph representation and data extraction

### Generate Input file for the pass:

> clang -g -S -emit-llvm code.c -Xclang -disable-O0-optnone -o code.ll

### Pass usage without extracting information from loops:

> path/to/opt -load /path/to/lib/LLVMFG.so -enable-new-pm=0 -printFGNL < code.ll > /dev/null

### Pass usage with information from loops:

> path/to/opt -load /path/to/lib/LLVMFG.so -enable-new-pm=0 -basic-aa -mem2reg -simplifycfg -loop-simplify -loop-rotate -simplifycfg -instcombine -indvars -da -analyze -printFG < code.ll > /dev/null

### Pass output:

- dot files that can be used to generate a picture of the FG of all the functions

- files that contains information about the functions in the code


### Instruction Data Format:

`[node_id, llvm_inst_opcode, num_operands, operands_size, [operand0_type, operand1_type..], is_array, array_size, in_loop]`

### Loop Data Format:

```
Loopid{
[loop_stride, trip_count, depth]
innerloopid{
[loop_stride, trip_count, depth]
[node_id, llvm_inst_opcode, num_operands, operands_size, [operand0_type, operand1_type..], is_array, array_size, in_loop]
[node_id, llvm_inst_opcode, num_operands, operands_size, [operand0_type, operand1_type..], is_array, array_size, in_loop]
...
end innerloopid}
[node_id, llvm_inst_opcode, num_operands, operands_size, [operand0_type, operand1_type..], is_array, array_size, in_loop]
[node_id, llvm_inst_opcode, num_operands, operands_size, [operand0_type, operand1_type..], is_array, array_size, in_loop]
...
...
end loopid}
```

NOTE: `loop_stride`, `trip_count`, `depth` are reliable only with additional pass (`-basic-aa -mem2reg -simplifycfg -loop-simplify -loop-rotate -simplifycfg -instcombine -indvars -da -analyze`)

### Stats Data Format:

```
FunctionName{
BB0[#nodes_in_bb0,in_loop_bb0,[(inst1:#inst1),(inst2:#inst2),..,(otherinst:0)]]
BB1[#nodes_in_bb1,in_loop_bb1,[(inst1:#inst1),(inst2:#inst2),..,(otherinst:0)]]
...
}
```

### Output Example: 

> dot -Tpng mainFG.dot -o mainFG.png

![mainFG](https://user-images.githubusercontent.com/26326254/130455742-3fdb5dc6-5a24-435e-bf75-a55116d8c6dc.png)

#### Color table:
- **Green Box**: Basic Blocks in a Loop
- **Blue Box**: Basic Blocks outside a Loop
- **Red Box**: Loop (Nested loops are darker)
- **Red edge**: Control Flow dependence
- **Black edge**: Data dependence
- **Gold edge**: Data dependence that refers to a function argument
- **Gold dashed edge**: Reference to the base address of an array/structure that is a function parameter
- **Darkgreen dashed edge**: Reference to the base address of an local array/structure 


> cat function_1_loopdata

```
Loop0 {
[3, 3, 1]
[119, 55, 2, 64, int, int, 0, -1, 1]
[120, 56, 4, 64, ptr, 0, -1, 1]
[121, 34, 3, 64, ptr, int, int, 0, -1, 1]
[122, 33, 2, 64, int, ptr, 0, -1, 1]
[123, 13, 2, 64, int, int, 0, -1, 1]
[124, 56, 4, 64, ptr, 0, -1, 1]
[125, 53, 2, 64, int, int, 0, -1, 1]
[126, 2, 3, 1, int, 0, -1, 1]
End Loop0 }
```


TODO:
- organize code in functions
- fix some bug that create wrong cdfg dot file
- ignore llvm debug instruction