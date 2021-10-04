# treeVM - Tree based bytecode VM




## Program & Data
### dpc-address
The dpc-address is used to get labels and literals in the program/data areas.
It is defined as unsigned 32-Bit number
```c
typedef uint32_t vm_dpc_address_t
```

### Loaders
The program and data area are both organized as an up to (2³² - 1) elements big block of memory
the VM expects that it doesnt change during runtime

### Interpreters
The program and data area are both organized as an up to (2¹⁶ - 1) chunks, each with a size of up to (2¹⁶ - 1) elements,
the address is split up into a chunk index by division and into an offset by modulus,
the vm expects that the individual chunks dont change, when reallocating the array of chunks the interpreter should lock the areas,
the vm uses the vmx_interpreter_lock and vmx_interpreter_unlock functions for synchronization.



### Data
#### Alignment & Addressing
Data types are aligned to 8-Bytes and addressed as 8-Byte elements

#### Small Numbers
32-Bit numbers are stored in the instruction itself, see int32, uint32 and float Data types

#### Big Numbers
Doubles are stored in the data area as a single 8-Byte element

#### Strings
Strings are stored in the data area starting at an 8-Byte element,
the first element contains the string header (`vm_string_t`),
the size field of the string header is ignored, so it is recommended that both fields are set to the length




## Opcodes
### Data types
See also enum vm_node_type
| Name        | Id | Insertable | Internal | Callable | Arithmetic | Additional information
| ------------|:--:| ---------- | -------- | -------- | ---------- | -------------
| null        | 0  | X          | X        |          |            | 
| int32       | 1  | X          |          |          | X          | will be converted to double (number)
| uint32      | 2  | X          |          |          | X          | will be converted to double (number)
| float       | 3  | X          |          |          | X          | will be converted to double (number)
| number      | 4  | X          | X        |          | X          | must be read from data area, using dpc_address
| string      | 5  | X          | X        |          |            | must be read from data area, using dpc_address
| value       | 6  | X          |          | X        | X          | Set to value of node, without children
| node        | 7  | X          |          |          |            | Set to value of node, with children, cant be set to the same node
| cut node    | 8  | X          |          | X        | X          | Set to value of node, with children, and remove original, cant be set to the same node
| pop node    | 9  | X          |          |          |            | Set to value of node in subtree, and remove last node, cant be done in the ins instruction
| function    | 12 | X          | X        | X        |            | Bytecode function
| core_func.  | 13 | X          | X        | X        |            | Core Library function
| ext_func.   | 14 | X          | X        | X        |            | Extended Library function
| dyn_func.   | 15 | X          | X        | X        |            | Dynamic Library function
| call        | 17 |            | X        |          |            |
| object ref. | 18 |            | X        |          |            | Object reference
| custom type | 19 |            | X        |          |            | Custom data type


#### Insertable Types
These are types that can be inserted through the ins, set, push ... instructions as a literal

#### Internal Types
Internal types are types that the vm can store, other types are converted

#### Callable Types
Callable types can be called with the Call instruction,
value and pop-node types are indirect and instead refer to a node with a callable type


#### Arithemtic Types
Arithmetic types can be used in the arithmetic instructions,
value and pop-node types are indirect and instead refer to a node with a callable type


### Format
Instructions are always 8 bytes long

| Symbol | Meaning 
|:----:| -------
| I    | Opcode, analogous to enum vm_opcode
| N    | Node byte, index to the node
| T    | Type byte, specifies the data type, see enum vm_node_type
| D    | General purpose data byte, awlway accompied by a T byte
| 0    | Unsigned integer byte
| J    | Byte for a jump address
| .    | Unused byte, set to 0


### Instructions
| Name   | Opcode | Format             | Description
| ------ | ------ | ------------------ | -----------
| exit   | 64     | I 0 . . . . . .    | Exit the program and return error, error is limited between 0 and 127
| vint   | 65     | I . 00 JJJJ        | Call interrupt with 00 as argument, and goto JJJJ if interrupt returns !0
|  |  |  |
| new    | 66     | I . NN . . . .    | Allocate a heap object and put reference into node
| free   | 67     | I . NN . . . .    | Free a heap object by its réference
| open   | 68     | I . NN . . . .    | Open a heap object, so that it can be entered
| close  | 69     | I . NN . . . .    | Close a open heap object
|  |  |  |
| slen   | 70     | I . NN NN . .    | Get string length
|  |  |  |
| push   | 71     | I T NN DDDD       | Insert node into N subtree
| ins    | 72     | I T .. DDDD       | Insert a new node
| set    | 73     | I T NN DDDD       | Set a node
| rem    | 74     | I . . . . . . .    | Remove the last node
| clear  | 75     | I . NN . . . .    | Clear a nodes children
| len    | 76     | I . NN . . . .    | Get length of current subtree
| enter  | 77     | I . NN . . . .    | Enter a subtree
| leave  | 78     | I . NN . . . .    | Leave the subtree
| pop    | 79     | I . NN NN . .    | Put last node in subtree into node, and clear source node
|  |  |  |
| jmp    | 80     | I . . . JJJJ       | Jump to JJJJ
| jtab   | 81     | I 0 NN JJJJ        | Jump table, skip instruction if value is higher than 0
| jneg   | 82     | I . NN JJJJ       | Jump to JJJJ if value is negative
| jzr    | 83     | I . NN JJJJ       | Jump to JJJJ if value is zero
| jpos   | 84     | I . NN JJJJ       | Jump to JJJJ if value is positive
| jnul   | 85     | I . NN JJJJ       | Jump to JJJJ if value is null
| call   | 86     | I T NN DDDD       | Call DDDD, the Operant must be a Callable types
| return | 87     | I . NN . . . .    | Return a node, to the caller
|  |  |  |
| add    | 88     | I T NN DDDD       | Add, the operant must be an Arithmetic type
| sub    | 89     | I T NN DDDD       | Subtract, the operant must be an Arithmetic type
| mul    | 90     | I T NN DDDD       | Multiply, the operant must be an Arithmetic type
| div    | 91     | I T NN DDDD       | Divide, the operant must be an Arithmetic type
| mod    | 92     | I T NN DDDD       | Modulo, the operant must be an Arithmetic type
| neg    | 93     | I . NN . . . .    | Negate (* -1)
| floor  | 94     | I . NN . . . .    | Floor, round down to next full number
| ceil   | 95     | I . NN . . . .    | Ceil, round up to next full number




## Nodes and Children
The program always starts in the root node, and can enter into child nodes, or leave child nodes,
only the children of the current nodes can be addressed by instructions.

### Enter
The enter instruction "enters" a child of the current node, and makes it the new current node

### Leave
The leave instruction leaves a node, and goes back to the parent node

### Apush and Apop
The Apush removes a node and appends it to an subtree, Apop removes the last node in the subtree and returns it




## Functions and Calls
### Arguments
The VM only allows the child nodes to be used as arguments

### Returns
The function can return a single node, that will take the place of the parent node




## Heap and Objects
### References
Heap objects can be opened by a reference, when a reference is copied it is alway registerd,
when deleting a reference, it is also registered,
when there are no more references left the object is freed


### Open Objects
When an object is opened it becomes part of the main tree, and gets synced back when closing,
therefore there can only be one open reference per object.
Open objects cant be moved in the tree,
if there is an open node in a cleared subtree, the node will be cleared and closed




### Heap Instructions
#### new
New creates an new object in the heap and puts reference into the specified node

#### free
Free frees an object in the heap, and invalidates the reference

#### open
Open a heap reference, so that it can be accessed by the node
It also stores the value of the node in the heap, so that it can be gotten back when closing

#### close
Closes a heap object, it sets the value of the node to the node stores in the heap




## Integration
How to integrate the vm into a loader/interpreter

### Include
The include.h file must be included


### Compilation
1. Compile the treevm.c file as an object file, `gcc treevm.c -c -options`
2. Compile the loader/interpreter as an object file `gcc loader.c -c -options`
3. Link the files together with `gcc -o output loader.o treevm.o`

output is a placeholder for the resulting binary file, loader.c/loader.o is a placeholder for the loader/interpreter


### Compiler Options
Clang and GCC provide the -D option which can be used to define a macro, during compilation,
The following macros can be set for the VM
| Name                 | Prupose   
| -------------------- | ------
| -DLOADER             | If defined use a loader
| -DINTERPRETER        | If defined use a interpreter
| -DFORCE_SWITCH       | Force the use of a switch, even if a jump table is available
| -DEXPLICIT_LHCAT     | Say that LHCAT is defined though a global variable, instead of the default of 8


### Globals
The following globals and functions must be defined so that the VM can be used


| Declaration                                               | Purpose
| --------------------------------------------------------- | -------
| vm_core_t vmx_core                                        | Contains the program and data
| void vmx_interpreter_lock(void)                           | Function, called by the interpreter when jumping/getting data
| void vmx_interpreter_unlock(void)                         | Function, equivalent to vmx_interpreter_unlock
| int vmx_interrupt(uint16_t int_id)                        | Function, interrupt that can be called by the program, jumps if it returns !0
| void vmx_return_clean(vm_node_t *node, vm_error_t error)  | Function, cleaning up the return value, subtree is cleared after
| vm_error_t (* const vmx_corelib[])(vm_node_t *)           | Library functions, functions can return !0 on error
| const uint32_t vmx_corelib_len                            | Length of corelib
| vm_error_t (* * vmx_extlib)(vm_node_t *)                  | Extended Library functions, same as corelib, but isnt constant at startup
| uint32_t vmx_extlib_len                                   | Extended Length of langlib
| vm_error_t vmx_dynlib(uint32_t fn_id, vm_node_t *node)    | Function, calling dynamic functions
| int vmx_dynlib_test(uint32_t fn_id)                       | Function, testing if a dynamic function is available
| const unsigned int VMX_LHCAT								| Only required with -DEXPLICIT_LHCAT flag, sets the least heap size category



The following globals are exposed to the loader/interpreter
| Declaration                                                          | Purpose
| -------------------------------------------------------------------- | -------
| const double VM_MAX_PRECISE_DOUBLE                                   | Biggest precise integer a double can represent
| int vm_get_build_information(volatile int no_print)                  | Print vm information, if no_print isnt set
| vm_error_t vm_run_main(const vm_instruction_t *ip, vm_node_t *root)  | Function for running the vm, also initializes the vm
| vm_error_t vm_call_node(const vm_instruction_t *ip, vm_node_t *root) | Function for running a function call
| vm_error_t vm_node_copy_value(vm_node_t *src, vm_node_t *trg)        | 
| vm_error_t vm_node_copy(vm_node_t *src, vm_node_t *trg)              | 
| vm_error_t vm_node_clear_value(vm_node_t *node)                      | 
| vm_error_t vm_node_clear_children(vm_node_t *node)                   | 
| vm_error_t vm_node_clear(vm_node_t *node, int preserve_open_object)  | 
| vm_error_t vm_heap_alloc(vm_node_t *node)                            | Allocate a heap object, also cleans the value of the node, subtree will remain
| vm_error_t vm_heap_free(vm_node_t *node)                             | Free a heap object, must be given an reference
| vm_error_t vm_heap_create_reference(vm_node_t *node)                 | Register a new object reference
| vm_error_t vm_heap_remove_reference(vm_node_t *node)                 | Remove a object reference
| vm_error_t vm_heap_load_object(vm_node_t *node)                      | Load a heap object, equivalent to open instruction
| vm_error_t vm_heap_unload_object(vm_node_t *node)                    | Unload a heap object, equivalent to close instruction
| vm_error_t vm_heap_verify(vm_node_t *node)                           | Check if an reference is valid
| vm_error_t vm_heap_verify_open(vm_node_t *node)                      | Check if an open object is valid

### Interrupt
The Interrupt gets called with the specified uint16 as an argument, if the interrupt returns !0 the program jumps to the specified address


### Library
The library is a set of non bytecode functions callable by the program, there are 3 seperate librarys

#### Core Library
The Corelib is provided by the interpreter/loader through vmx_corelib and vmx_corelib_len, the VM expects that the langlib ist static and doesnt change,
the functions get the node as an argument,
the function can return !0 causing the program to crash

#### Language Library
The Langlib is provided by the interpreter/loader through vmx_langlib and vmx_langlib_len,
the same as corelib, but isnt constant at startup, so it can be swapped before the start of the vm,
the library needs to be static during the runtime

#### Dynamic Library
The Dynamic Library is accessed through the vmx_dynlib function, which gets the function id and the node as arguments,
the vmx_dynlib function can return an error.
The vmx_dynlib_test function returns !0 if a dynamic function doesn't exist
