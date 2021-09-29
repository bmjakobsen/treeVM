/*
Define vm options with -Doption
-DLOADER					Use a loader
-DINTERPRETER				Use a Intepreter

-DFORCE_SWITCH				Force using a switch, even if a jump table is available

-DVM_LHAREA=x				Least heap area size
*/


#if (!defined __STDC_VERSION__) || (__STDC_VERSION__ < 199901L)		/* Test for c99 */
	#error treevm requires c99 or above
#endif


/* -D Compiler Options */
#undef VM_INTERPRETER
#if (!defined LOADER) && (!defined INTERPRETER)
	#warning Either -DLOADER or -DINTERPRETER should be set, treeVM is now compiled in loader mode by default
#elif (defined LOADER) && (defined INTERPRETER)
	#error -DLOADER and -DINTERPRETER cant be set at the same time
#elif defined LOADER
	#undef LOADER
#elif defined INTERPRETER
	#undef INTERPRETER
	#define VM_INTERPRETER 1
#endif



//Test for GCC or Clang, for use of jump table
#undef VM_GOTO_MODE
#ifndef FORCE_SWITCH
	/* Check for gcc, which added labels as values in 3.4.0, but only allow 4+ */
	#if (defined __GNUC__ && ! defined __CLANG__) && (__GNUC__ >= 4)
		#define VM_GOTO_MODE 1
		
		#define LABEL(opcode) (&&opcode)
		#define GOTO(ptr_to_label) goto *ptr_to_label
		
		#define VM_NOINLINE __attribute__((noinline, noclone))
		#define VM_USED __attribute__((used))
	#endif
	
	/* Check for clang 3.0+ */
	#if (defined __CLANG__) && (__CLANG__ >= 3)
		#define VM_GOTO_MODE 2
		
		#define LABEL(opcode) (&&opcode)
		#define GOTO(ptr_to_label) goto *ptr_to_label
		
		#define VM_NOINLINE __attribute__((noinline, noclone))
		#define VM_USED __attribute__((used))
	#endif
#endif

#ifndef VM_GOTO_MODE
	#define VM_NOINLINE
	#define VM_USED
#endif



#define TREEVM_MAIN_SOURCE_FILE 1



/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#undef TREEVM_INCLUDE_H_
#include "./include.h"




/* Macros */
#define VM_NODES_STARTING_SIZE 8


#define MACRO_STRING2(x) #x
#define MACRO_STRING(x) MACRO_STRING2(x)


#define VM_PANIC(code) {\
	error2 = code;\
	goto stop_l;\
}

#define VM_NULL_VALUE (union vm_node_value) {0}
#define VM_NODE_INITIALIZER(type2) ((vm_node_t) { .value = VM_NULL_VALUE, .object_address = 0, .type = type2, .subtree = NULL })
#define SFREE(x) free(x); x = NULL;
#define SMALLOC(x) malloc(x);
#define SREALLOC(x, y) realloc(x, y)


#ifdef SIZE_MAX
	#define VM_SIZE_MAX SIZE_MAX
#else
	#define VM_SIZE_MAX UINTPTR_MAX
#endif



VM_USED
static volatile const char vm_build_information_string[] = 
	"  \x06             "
	"\n--------------\n"
	"| "__DATE__"  \n"
	"| "__TIME__"     \n"
	"|              \n"
	"| Version 1.0  \n"
	"| MIT LICENSE  \n"
	"+--------------\n"
	"                ";



//Get the max size of a subtree
#define VM_SUBTREE_MAX_SIZE2 ((VM_SIZE_MAX - sizeof(struct vm_node_subtree)) / sizeof(vm_node_t))		//The max the size_t type can gold
static const uint32_t VM_SUBTREE_MAX_COUNT = (0x80000000 > VM_SUBTREE_MAX_SIZE2) ? ((uint32_t) VM_SUBTREE_MAX_SIZE2) : 0x80000000; 
#undef VM_SUBTREE_MAX_SIZE2





/* Globals */
const double VM_MAX_PRECISE_DOUBLE = ((double) ((((uint64_t) 2)<<51) - 1));
static int running = 0;


//Includes, clear inclusion guards
#undef TREEVM_HEAP_H_
#undef TREEVM_FUNCTIONS_H_
#include "./heap.h"
#include "./functions.h"




//Ignore this, this is for tricking the intellisense
#ifndef VM_LHAREA_C
	#define VM_LHAREA_C 32
#endif




//Get Macro definitions for opcodes
#ifdef VM_GOTO_MODE
	#define OPCODE_UNDEF1_L LABEL(opcode_undef_l)
	#define OPCODE_UNDEF8_L OPCODE_UNDEF1_L, OPCODE_UNDEF1_L, OPCODE_UNDEF1_L, OPCODE_UNDEF1_L, OPCODE_UNDEF1_L, OPCODE_UNDEF1_L, OPCODE_UNDEF1_L, OPCODE_UNDEF1_L
	#define OPCODE_UNDEF16_L OPCODE_UNDEF8_L, OPCODE_UNDEF8_L
	
	
	#define OPCODE_LIST_HEAD	OPCODE_UNDEF16_L, OPCODE_UNDEF16_L, OPCODE_UNDEF16_L, OPCODE_UNDEF16_L
	
	#define OPCODE_L(op) [op]=LABEL(opcode_##op##_l)
	//#define OPCODE_U(op) [op]=LABEL(opcode_undef_l)
	
	#define OPCODE_LIST_BODY\
		OPCODE_L(VM_OPCODE_EXIT),	OPCODE_L(VM_OPCODE_VINT),	OPCODE_L(VM_OPCODE_NEW),	OPCODE_L(VM_OPCODE_FREE),\
		OPCODE_L(VM_OPCODE_OPEN),	OPCODE_L(VM_OPCODE_CLOSE),	OPCODE_L(VM_OPCODE_SLEN),	OPCODE_L(VM_OPCODE_PUSH),\
		OPCODE_L(VM_OPCODE_INS),	OPCODE_L(VM_OPCODE_SET),	OPCODE_L(VM_OPCODE_REM),	OPCODE_L(VM_OPCODE_CLEAR),\
		OPCODE_L(VM_OPCODE_LEN),	OPCODE_L(VM_OPCODE_ENTER),	OPCODE_L(VM_OPCODE_LEAVE),	OPCODE_L(VM_OPCODE_POP),\
		OPCODE_L(VM_OPCODE_JMP),	OPCODE_L(VM_OPCODE_JTAB),	OPCODE_L(VM_OPCODE_JNEG),	OPCODE_L(VM_OPCODE_JZR),\
		OPCODE_L(VM_OPCODE_JPOS),	OPCODE_L(VM_OPCODE_JNUL),	OPCODE_L(VM_OPCODE_CALL),	OPCODE_L(VM_OPCODE_RETURN),\
		OPCODE_L(VM_OPCODE_ADD),	OPCODE_L(VM_OPCODE_SUB),	OPCODE_L(VM_OPCODE_MUL),	OPCODE_L(VM_OPCODE_DIV),\
		OPCODE_L(VM_OPCODE_MOD),	OPCODE_L(VM_OPCODE_NEG),	OPCODE_L(VM_OPCODE_FLOOR),	OPCODE_L(VM_OPCODE_CEIL)
	
	
	#define OPCODE_LIST_TAIL	OPCODE_UNDEF16_L, OPCODE_UNDEF16_L, OPCODE_UNDEF16_L, OPCODE_UNDEF16_L,\
								OPCODE_UNDEF16_L, OPCODE_UNDEF16_L, OPCODE_UNDEF16_L, OPCODE_UNDEF16_L,\
								OPCODE_UNDEF16_L, OPCODE_UNDEF16_L
#endif


VM_NOINLINE
static vm_error_t vm_call_node2(const vm_instruction_t * const ip_call, vm_node_t * const parent_call) {
	vm_error_t error2 = 0;
	
	
	const vm_instruction_t * restrict ip = ip_call;
	vm_node_t * restrict node = parent_call;
	vm_node_t * restrict parent = NULL;		/* temporal parent pointer for wehen entering a subnode without parent pointer */
	
	uint32_t len = 0;
	uint32_t size = 0;
	struct vm_node_subtree * restrict subtree = node->subtree;
	
	if (subtree != NULL) {
		len = subtree->len;
		size = subtree->size;
	}
	
	
	
	#ifdef VM_GOTO_MODE
	static void * const restrict opcodes[256] = {OPCODE_LIST_HEAD, OPCODE_LIST_BODY, OPCODE_LIST_TAIL};
	#endif
	
	goto recontinue2_l;
	recontinue_l:
		ip++;
	
	recontinue2_l:
	#ifdef VM_GOTO_MODE
		GOTO(opcodes[* (vm_opcode_t*) ip]);
	#else
		switch(* (vm_opcode_t*) ip)
	#endif
		{
			#undef TREEVM_OPCODES_H_
			#include "./opcodes.h"
		}
	
	
	stop_l:
	
	
	return(error2);
}



vm_error_t vm_call_node(const vm_instruction_t * const ip, vm_node_t * const root) {
	if (running == 0) {
		return(VM_ERROR_VM_NOT_RUNNING);
	}

	if (root->subtree != NULL) {
		root->subtree->parent = NULL;
	}
	root->type = VM_TYPE_CALL;

	return(vm_call_node(ip, root));
}


VM_USED
vm_error_t vm_run_main(const vm_instruction_t * const restrict ip, vm_node_t * const restrict root) {
	vm_error_t error;


	//Trick the compiler to not delete a variable
	vm_print_information(1);
	if (vm_build_information_string[2] != '\x06') {
		return(VM_ERROR_NO_0X06);
	}


	if ((error = vm_heap_init()))
		return(error);
	

	running = 1;

	error = vm_call_node(ip, root);

	running = 0;


	//Clean up the returned value
	vmx_return_cleanup(root, error);
	vm_node_clear_children(root);
	
	
	//Free the heap
	vm_heap_destroy();
	
	
	return(error);
}



VM_USED
void vm_print_information(volatile int no_print) {
	if (no_print)
		return;
	
	printf("Time: %s\n", __TIME__);
	printf("Date: %s\n", __DATE__);
	printf("\\x06: %c 0x%.2x\n\n", vm_build_information_string[2], vm_build_information_string[2]);
	
	printf("Build information:\n%s\n\n", vm_build_information_string);

	printf("Data Size:\n");
	printf("\tProgram: %u\n", (unsigned int) sizeof(struct vm_core));
	printf("\tHeap: %u\n", (unsigned int) sizeof(struct vm_heap));
	printf("\tHeap Node: %u\n", (unsigned int) sizeof(struct vm_heap_node));
	printf("\tNode: %u\n", (unsigned int) sizeof(vm_node_t));
	printf("\tSubtree: %u\n", (unsigned int) sizeof(struct vm_node_subtree));

	printf("Heap Area tests:\n");
	printf("\tHeap area: Count:%lu Max:%lu\n", (unsigned long) VM_LHAREA_C, (unsigned long) vm_heap_get_area(VM_HEAP_MAX_COUNT - 1));
}
