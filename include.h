#ifndef TREEVM_INCLUDE_H_
#define TREEVM_INCLUDE_H_ 1




#include <limits.h>
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>



#define VM_CONCAT(a, b) VM_CONCAT1(a, b)
#define VM_CONCAT1(a, b) a##b



typedef enum vm_error {
	VM_ERROR_NONE = 0,
	VM_ERROR_EXITED = 0,
	
	//General Errors (128 - 143)
	VM_ERROR_INIT_OUT_OF_MEMORY = 128,
	VM_ERROR_OUT_OF_MEMORY = 129,
	VM_ERROR_VM_NOT_RUNNING = 130,
	

	//Heap Errors (144 - 159)
	VM_ERROR_NO_SUCH_OBJECT = 144,
	VM_ERROR_OBJECT_OUT_OF_PHASE = 145,
	VM_ERROR_LOADED_OBJECT = 146,
	VM_ERROR_UNLOADED_OBJECT = 147,
	VM_ERROR_HEAP_REACHED_MAX_SIZE = 148,


	//Undefined (160 - 175)
	//


	//Code Errors (176 - 223)
	VM_ERROR_INVALID_OPCODE = 176,

	VM_ERROR_OUT_OF_RANGE_NODE = 178,
	VM_ERROR_SUBTREE_REACHED_MAX_SIZE = 179,

	VM_ERROR_UNKNOWN_OR_INVALID_TYPE = 182,

	VM_ERROR_FAILED_TO_COPY_CUSTOM_TYPE = 183,

	VM_ERROR_NODE_WITHOUT_PARENT = 184,


	VM_ERROR_EXPECTED_NUMBER = 192,
	VM_ERROR_EXPECTED_STRING = 193,
	VM_ERROR_EXPECTED_OBJECT_REFERENCE = 194,
	VM_ERROR_EXPECTED_CUSTOM_TYPE = 195,
	VM_ERROR_EXPECTED_CALLABLE_TYPE = 196,
	VM_ERROR_EXPECTED_SUBTREE = 197,

	VM_ERROR_INVALID_JUMP_ADDRESS = 200,
	VM_ERROR_INVALID_CORE_FUNCTION = 201,
	VM_ERROR_INVALID_EXTENDED_FUNCTION = 202,
	VM_ERROR_INVALID_DYNAMIC_FUNCTION = 203,
	VM_ERROR_FAILED_FUNCTION_CALL = 204,


	VM_ERROR_INVALID_NUMBER_FOR_INDEXING = 208,
	VM_ERROR_ARITHMETIC_ERROR = 209,
} vm_error_t;



/* Enums */
enum vm_opcode {
	/* Opcodes start at 48 '0' to 111 ('o')  (64 instructions, including 'i')
		Instructions:
		
		T: Type byte: enum VM_TYPE_BYTE
		D: Data byte
		J: Address byte
		0: Integer byte
		N: Node byte
		.: Unused
	*/
	
	
	
	/* Misc: */
	VM_OPCODE_EXIT = 64,		//exit		I 0 ...... X		Exit and return error, if F return value of node as error, else return value of 0
	VM_OPCODE_VINT = 65,		//vint		I . 00 JJJJ X		Interrupt, jump to JJJJ if returns !0, given 0 will terminate
		
	VM_OPCODE_NEW = 66,			//new		I N ...... X		Create object
	VM_OPCODE_FREE = 67,		//free		I N ...... X		Free object
	
	VM_OPCODE_OPEN = 68,
	VM_OPCODE_CLOSE = 69,
		
	VM_OPCODE_SLEN = 70,		//slen		I N N ..... X		Get string length
	VM_OPCODE_PUSH = 71,		//ains		I N . T DDDD X		Insert a node into a subtree
	
	
	/* Node: */
	VM_OPCODE_INS = 72,			//ins		I . O T DDDD X	Create new node
	VM_OPCODE_SET = 73,			//set		I N O T DDDD X	Set node to value
	VM_OPCODE_REM = 74,			//rem		I ....... X		Remove a node
	VM_OPCODE_CLEAR = 75,		//clear		I N ...... X	Clear children
	
	VM_OPCODE_LEN = 76,			//len		I N ...... X	Get length of current subtree
	
	VM_OPCODE_ENTER = 77,		//enter		I N ...... X		Enter a subtree, count the number if insert instruction after enter and allocate a fitting number of nodes
	VM_OPCODE_LEAVE = 78,		//leave		I ....... X			Leave a subtree
	
	VM_OPCODE_POP = 79,		//arem		I N N ..... X		Remove last element

	
	/* Branching: */
	VM_OPCODE_JMP = 80,			//jmp		I ... JJJJ X	Jump
	VM_OPCODE_JTAB = 81,		//jtab		I N 00 JJJJ X		Jump table, if higher than 00 skip instruction
		
	VM_OPCODE_JNEG = 82,		//jneg		I N .. JJJJ X		Jump if node is negative
	VM_OPCODE_JZR = 83,			//jzr		I N .. JJJJ X	Jump if node is zero
	VM_OPCODE_JPOS = 84,		//jpos		I N .. JJJJ X		Jump if node is positive
	VM_OPCODE_JNUL = 85,		//jnul		I N .. JJJJ X		Jump if node is null
		
	VM_OPCODE_CALL = 86,		//call		I N . T DDDD X		Call a function on the node to address
	VM_OPCODE_RETURN = 87,		//return	I N ...... X		Return a node	
	
	/* Math: */
	VM_OPCODE_ADD = 88,			//add		I N . T DDDD X	Add
	VM_OPCODE_SUB = 89,			//sub		I N . T DDDD X	Subtract
	VM_OPCODE_MUL = 90,			//mul		I N . T DDDD X	Multiply
	VM_OPCODE_DIV = 91,			//div		I N . T DDDD X	Division
	VM_OPCODE_MOD = 92,			//mod		I N . T DDDD X	Modulus
	VM_OPCODE_NEG = 93,			//neg		I N ...... X	Negate
	VM_OPCODE_FLOOR = 94,		//down		I N ...... X		Round to last full number down
	VM_OPCODE_CEIL = 95,		//ceil		I N ...... X		Round to last full number up
};



enum vm_node_type {
	VM_TYPE_NULL = 0,					// Sets the node to value and removes children
	VM_TYPE_INT32 = 1,
	VM_TYPE_UINT32 = 2,
	VM_TYPE_FLOAT = 3,
	VM_TYPE_DOUBLE = 4,
	VM_TYPE_STRING = 5,
	
	VM_TYPE_VALUE = 6,					// Copys node value
	VM_TYPE_NODE = 7,					// Copys node and children, children are excluded if node is object reference
	VM_TYPE_CUT_NODE = 8,				//Moves nodes and children, nulls the original node
	VM_TYPE_POP_NODE = 9,				// Moves last node in subtree, and removes the last node

	VM_TYPE_BYTECODE_FUNCTION = 12,		// Doesnt remove children
	VM_TYPE_CORE_FUNCTION = 13,			
	VM_TYPE_EXTENDED_FUNCTION = 14,		
	VM_TYPE_DYNAMIC_FUNCTION = 15,		
	
	
	
	VM_TYPE_NUMBER = 4,
	
	
	//Internal types, cant be inserted because their id is out of the 8 bit range
	VM_TYPE_CALL = 17,
	
	VM_TYPE_OBJECT_REFERENCE = 18,
	VM_TYPE_CUSTOM_TYPE = 19,
};




/* Types */
typedef union vm_dpc_address {
	uint32_t ld;
	uint16_t it[2];
} vm_dpc_address_t;


typedef uint64_t vm_instruction_t;
typedef uint8_t vm_opcode_t;
typedef uint8_t vm_type_t;
typedef vm_instruction_t vm_data_unit_t;




typedef struct vm_string {
	uint32_t len, size;
	char c[];
} vm_string_t;


typedef struct vm_heap_object {
	uint32_t address;
	uint32_t phase;
} vm_object_t;


typedef struct vm_custom_type {
	uint32_t size;
	struct vm_custom_type *(*copy)(struct vm_custom_type *);
	void (*free)(struct vm_custom_type *);
	unsigned char data[];
} vm_custom_type_t;





typedef struct vm_node {
	union vm_node_value {
		double number;
		vm_string_t *string;
		
		vm_object_t object;
		vm_custom_type_t *custom_type;
		
		uint32_t function;
	} value;
	
	/* Check if node is value or subtree by checking subtree == NULL */
	struct vm_node_subtree *subtree;
	
	enum vm_node_type type;			/* Type of the node */
	uint32_t object_address;		/* Address of object if object is currently loaded */
} vm_node_t;


struct vm_node_subtree {
	uint32_t len, size;		/* Number/Size of children */
	
	
	vm_node_t *parent;		/* Parent node */
	
	
	vm_node_t child[];
};



typedef struct vm_core {
	struct vm_core_loader {
		vm_instruction_t * restrict program;
		vm_data_unit_t * restrict data;
		uint32_t program_len, data_len;
	} ld;
	volatile struct vm_core_interpreter {
		struct vm_data_chunk {
			vm_data_unit_t * restrict p;
			uint16_t len;
		} * program;
		struct vm_data_chunk * data;
		uint16_t program_len, data_len;
		
		int resizing;
		int accessing;
	} it;
} vm_core_t;





extern vm_core_t vmx_core;

extern void vmx_interpreter_lock(void);
extern void vmx_interpreter_unlock(void);

extern int vmx_interrupt(uint16_t int_id);
extern void vmx_return_cleanup(vm_node_t * node, vm_error_t error);

extern vm_error_t (* const vmx_corelib[])(vm_node_t *);
extern const uint32_t vmx_corelib_len;

extern vm_error_t (* * vmx_extlib)(vm_node_t *);
extern uint32_t vmx_extlib_len;

extern vm_error_t vmx_dynlib (uint32_t fn_id, vm_node_t *node);			//Runs dynamic function with node as argument
extern int vmx_dynlib_test (uint32_t fn_id);						//Returns !0 if the dynamic function doesnt exist

extern vm_error_t vmx_provide_heap_size(uint32_t size);			//Gets called when the heap is resized to provide the new heap size for the loader




//Globals exported
extern const double VM_MAX_PRECISE_DOUBLE;


//Functions exported
extern void vm_print_information(void);
extern vm_error_t vm_call_node(const vm_instruction_t *ip, vm_node_t *root);
extern vm_error_t vm_run_main(const vm_instruction_t *ip, vm_node_t *root);


//Node functions that are exported
extern vm_error_t vm_node_copy_value(vm_node_t *src, vm_node_t *trg);
extern vm_error_t vm_node_copy(vm_node_t *src, vm_node_t *trg);
extern vm_error_t vm_node_clear_value(vm_node_t *node);
extern vm_error_t vm_node_clear_children(vm_node_t *node);
extern vm_error_t vm_node_clear(vm_node_t *node, int preserve_open_object);

//Heap functions that are exported
extern vm_error_t vm_heap_alloc(vm_node_t *node);
extern vm_error_t vm_heap_free(vm_node_t *node);
extern vm_error_t vm_heap_create_reference(vm_node_t *node);
extern vm_error_t vm_heap_remove_reference(vm_node_t *node);
extern vm_error_t vm_heap_load_object(vm_node_t *node);
extern vm_error_t vm_heap_unload_object(vm_node_t *node);
extern vm_error_t vm_heap_verify(vm_node_t *node);
extern vm_error_t vm_heap_verify_open(vm_node_t *node);


#endif//TREEVM_INCLUDE_H_
