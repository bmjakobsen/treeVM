#ifndef TREEVM_MAIN_SOURCE_FILE
	#error This file must not be included
#endif




#ifndef TREEVM_OPCODES_H_
#define TREEVM_OPCODES_H_ 1




#ifdef VM_GOTO_MODE
	#define OPCODE_START(index) opcode_##index##_l:
#else
	#define OPCODE_START(index) case(index): 
#endif

#define VM_GET_ARG(type, off) (*((type*) (((const uint8_t * restrict) &ip) + off)))



#define VM_INSTRUCTION_TYPE  VM_GET_ARG(uint8_t, 1)
#define VM_INSTRUCTION_UINT8  VM_GET_ARG(uint8_t, 1)
#define VM_INSTRUCTION_INTID VM_GET_ARG(uint16_t, 2)
#define VM_INSTRUCTION_NODE VM_GET_ARG(uint16_t, 2)
#define VM_INSTRUCTION_NODE2 VM_GET_ARG(uint16_t, 4)
#define VM_INSTRUCTION_OPERANT(type) VM_GET_ARG(type, 4)

#define VM_INSTRUCTION_GET(x) VM_INSTRUCTION_##x





OPCODE_START(VM_OPCODE_EXIT) {
	VM_PANIC(VM_INSTRUCTION_GET(UINT8)&0x7f);
	
	goto recontinue2_l;
}
OPCODE_START(VM_OPCODE_VINT) {
	//Call interrupt
	if (vmx_interrupt(VM_INSTRUCTION_GET(INTID))) {
		//Get jump address and jump
		vm_instruction_t *tj = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
		if (tj == NULL) {
			VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
		}
		
		ip = tj;
		goto recontinue2_l;
	}
		
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_NEW) {
	vm_error_t lerror;
	
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Allocate a new node, also clears value
	if ((lerror = vm_heap_alloc(&subtree->child[VM_INSTRUCTION_GET(NODE)]))) {
		VM_PANIC(lerror);
	}
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_FREE) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check type of the node
	vm_error_t lerror;
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type == VM_TYPE_OBJECT_REFERENCE) {
		//free node
		if ((lerror = vm_heap_free(&subtree->child[VM_INSTRUCTION_GET(NODE)]))) {
			VM_PANIC(lerror);
		}
	} else {
		VM_PANIC(VM_ERROR_EXPECTED_OBJECT_REFERENCE);
	}
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_OPEN) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Load node
	vm_error_t lerror;
	if ((lerror = vm_heap_load_object(&subtree->child[VM_INSTRUCTION_GET(NODE)]))) {
		VM_PANIC(lerror);
	}

	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_CLOSE) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Unload node
	vm_error_t lerror;
	if ((lerror = vm_heap_unload_object(&subtree->child[VM_INSTRUCTION_GET(NODE)]))) {
		VM_PANIC(lerror);
	}
	
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_SLEN) {
	vm_error_t lerror;
	
	if (VM_INSTRUCTION_GET(NODE) >= len || VM_INSTRUCTION_GET(NODE2) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check the type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_STRING) {
		VM_PANIC(VM_ERROR_EXPECTED_STRING);
	}
	
	//Clear value
	if ((lerror = vm_node_clear_value(&subtree->child[VM_INSTRUCTION_GET(NODE2)]))) {
		VM_PANIC(lerror);
	}
	
	//Get the length of the string
	subtree->child[VM_INSTRUCTION_GET(NODE2)].type = VM_TYPE_NUMBER;
	subtree->child[VM_INSTRUCTION_GET(NODE2)].value.number = subtree->child[VM_INSTRUCTION_GET(NODE)].value.string->len;
	
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_PUSH) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	
	//Check size of subtree
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree == NULL) {
		//Allocate subtree
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree = SMALLOC(sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * VM_NODES_STARTING_SIZE));
		if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree == NULL) {
			VM_PANIC(VM_ERROR_OUT_OF_MEMORY);
		}
		
		//Update values in subtree
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len = 0;
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size = VM_NODES_STARTING_SIZE;
	} else if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len >= subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size) {
		//Reallocate subtree
		if ((subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size * 2) > VM_SUBTREE_MAX_COUNT) {
			VM_PANIC(VM_ERROR_SUBTREE_REACHED_MAX_SIZE);
		}
		
		void *temp = SREALLOC(subtree->child[VM_INSTRUCTION_GET(NODE)].subtree, sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size * 2));
		if (temp == NULL) {
			VM_PANIC(VM_ERROR_OUT_OF_MEMORY);
		}
		
		//Update values in subtree
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree = temp;
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size *= 2;
	}
	
	//Set value of node
	vm_error_t lerror;
	if ((lerror = vm_node_set(VM_INSTRUCTION_GET(TYPE), subtree->child[VM_INSTRUCTION_GET(NODE)].subtree, &subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->child[subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len], &VM_INSTRUCTION_GET(OPERANT)(uint32_t)))) {
		VM_PANIC(lerror);
	}
	subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len++;	
	
	
	goto recontinue_l;
}


OPCODE_START(VM_OPCODE_INS) {
	//Check the length of the subtree, and reallocate if necessary
	if (len == 0) {
		//Allocate
		subtree = SMALLOC(sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * VM_NODES_STARTING_SIZE));
		if (subtree == NULL) {
			VM_PANIC(VM_ERROR_OUT_OF_MEMORY);
		}
		
		//Set values of subtree
		node->subtree = subtree;
		size = VM_NODES_STARTING_SIZE;
		subtree->size = size;
		subtree->len = 0;
		
		//subtree->parent = parent;
	} else if (len >= size) {
		if ((size * 2) > VM_SUBTREE_MAX_COUNT) {	//Check if allocation is too big for system
			VM_PANIC(VM_ERROR_SUBTREE_REACHED_MAX_SIZE);
		}
		//Reallocate subtree
		void *temp = SREALLOC(subtree, sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * size * 2));
		if (temp == NULL) {
			VM_PANIC(VM_ERROR_OUT_OF_MEMORY);
		}
		
		//Set values of subtree
		subtree = temp;
		node->subtree = subtree;
		size *= 2;
		subtree->size = size;
	}
	
	
	//Set value of node
	vm_error_t lerror;
	if ((lerror = vm_node_set(VM_INSTRUCTION_GET(TYPE), subtree, &subtree->child[len], &VM_INSTRUCTION_GET(OPERANT)(uint32_t)))) {
		VM_PANIC(lerror);
	}
	len++;
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_SET) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Set node to value
	vm_error_t lerror;
	if ((lerror = vm_node_set(VM_INSTRUCTION_GET(TYPE), subtree, &subtree->child[VM_INSTRUCTION_GET(NODE)], &VM_INSTRUCTION_GET(OPERANT)(uint32_t)))) {
		VM_PANIC(lerror);
	}
	
		
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_REM) {
	vm_error_t lerror;
	
	if (len == 0) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Clear nodes
	if ((lerror = vm_node_clear(&subtree->child[len - 1], 0))) {
		VM_PANIC(lerror);
	}
	len--;
	
	//Reallocate to a smaller size if necessary
	if (size > VM_NODES_STARTING_SIZE && len < (size / 2)) {
		void *newp = SREALLOC(subtree, sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * size / 2));
		if (newp == NULL)
			goto recontinue_l;
		
		//Set values of subtree
		subtree = newp;
		node->subtree = subtree;
		size /= 2;
		subtree->size = size;
	}
	
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_CLEAR) {
	vm_error_t lerror;
	
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Clear subtree of node
	if ((lerror = vm_node_clear_children(&subtree->child[VM_INSTRUCTION_GET(NODE)]))) {
		VM_PANIC(lerror);
	}
	
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_LEN) {
	vm_error_t lerror;
	
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	if ((lerror = vm_node_clear_value(&subtree->child[VM_INSTRUCTION_GET(NODE)]))) {
		VM_PANIC(lerror);
	}
	
	
	//Get the length of the string
	subtree->child[VM_INSTRUCTION_GET(NODE)].type = VM_TYPE_NUMBER;
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number = (double) len;
	
	
	goto recontinue_l;
}



OPCODE_START(VM_OPCODE_ENTER) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	
	//Update subtree and node node to local variable
	subtree->len = len;
	subtree->parent = parent;
	
	//Set node pointer
	parent = node;
	
	
	//Get new node pointer and set subtree
	node = &subtree->child[VM_INSTRUCTION_GET(NODE)];
	subtree = node->subtree;
	
	if (subtree != NULL) {
		len = subtree->len;
		size = subtree->size;
	} else {
		len = 0;
		size = 0;
	}
	
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_LEAVE) {
	node->subtree = subtree;
	
	if (len > 0) {
		subtree->len = len;
	} else if (parent == NULL) {
		VM_PANIC(VM_ERROR_NODE_WITHOUT_PARENT);
	}
	
	node = parent;
	subtree = node->subtree;
	
	parent = subtree->parent;
	
	len = subtree->len;
	size = subtree->size;
	
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_POP) {
	vm_error_t lerror;
	
	if (VM_INSTRUCTION_GET(NODE) >= len || VM_INSTRUCTION_GET(NODE2) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree == NULL || subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len == 0) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Clear target node
	if ((lerror = vm_node_clear(&subtree->child[VM_INSTRUCTION_GET(NODE2)], 1))) {
		VM_PANIC(lerror);
	}

	{	//Cut out node
		uint32_t arg = len - 1;
		if ((lerror = vm_node_set(VM_TYPE_CUT_NODE, subtree->child[VM_INSTRUCTION_GET(NODE)].subtree, &subtree->child[VM_INSTRUCTION_GET(NODE2)], &arg))) {
			VM_PANIC(lerror);
		}
	}

	//Clear node in subtree
	if ((lerror = vm_node_clear(&subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->child[len - 1], 0))) {
		VM_PANIC(lerror);
	}
	len--;

	//Reallocate subtree to smaller size if necessary
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len == 0) {
		SFREE(subtree->child[VM_INSTRUCTION_GET(NODE)].subtree);
	} else if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->len < (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size / 2)) {
		void *temp = SREALLOC(subtree->child[VM_INSTRUCTION_GET(NODE)].subtree, sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size / 2));
		if (temp == NULL)
			goto recontinue_l;
		
		//Set values in subtree
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree = temp;
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->size /= 2;
	}
	
	
	goto stop_l; goto recontinue_l;
}


OPCODE_START(VM_OPCODE_JMP) {
	//Get label and jump
	vm_instruction_t *nip = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
	if (nip == NULL) {
		VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
	}
	
	ip = nip;
	
	goto recontinue2_l;
}
OPCODE_START(VM_OPCODE_JTAB) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Get index for jump table
	double index2 = subtree->child[VM_INSTRUCTION_GET(NODE)].value.number;
	if (index2 > VM_MAX_PRECISE_DOUBLE || index2 < 0 || !(index2 != floor(index2))) {
		VM_PANIC(VM_ERROR_INVALID_NUMBER_FOR_INDEXING)
	}
	int64_t index = (int64_t) index2;
	
	//Skip if index is higher than uint16
	if (index >= VM_INSTRUCTION_GET(UINT8)) {
		goto recontinue_l;
	}
	
	//Get label
	vm_instruction_t *nip = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
	if (nip == NULL) {
		VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
	}
	
	//Jump to label + jump offset
	ip = nip + index;
	
	
	goto recontinue2_l;
}

OPCODE_START(VM_OPCODE_JNEG) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].value.number < 0.0) {
		//Get label and jump
		vm_instruction_t *nip = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
		if (nip == NULL) {
			VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
		}
		
		ip = nip;
		goto recontinue2_l;
	}
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_JZR) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].value.number == 0.0) {
		//Get label and jump
		vm_instruction_t *nip = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
		if (nip == NULL) {
			VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
		}
		
		ip = nip;
		goto recontinue2_l;
	}
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_JPOS) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].value.number > 0.0) {
		//Get label and jump
		vm_instruction_t *nip = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
		if (nip == NULL) {
			VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
		}
		
		ip = nip;
		goto recontinue2_l;
	}
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_JNUL) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type == VM_TYPE_NULL) {
		//Get label and jump
		vm_instruction_t *nip = vm_get_label(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
		if (nip == NULL) {
			VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
		}
		
		ip = nip;
		goto recontinue2_l;
	}
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_CALL) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	
	uint32_t function;
	union vm_dpc_address function2;
	vm_instruction_t *nip;
	vm_error_t call_error = 0;
	uint32_t object_address2 = subtree->child[VM_INSTRUCTION_GET(NODE)].object_address;
	

	//Set values in subtree to null
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree != NULL) {
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->parent = NULL;
	}
	subtree->child[VM_INSTRUCTION_GET(NODE)].object_address = 0;


	
	switch ((enum vm_node_type) VM_INSTRUCTION_GET(TYPE)) {
		//Indirect jumping, get jump address from node
		case(VM_TYPE_VALUE): 
			switch ((enum vm_node_type) subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].type) {
				case(VM_TYPE_BYTECODE_FUNCTION):
					function = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.function;
					goto vm_opcode_local_call;
				case(VM_TYPE_CORE_FUNCTION):
					function = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.function;
					goto vm_opcode_local_call_core;
				case(VM_TYPE_EXTENDED_FUNCTION):
					function = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.function;
					goto vm_opcode_local_call_library;
				case(VM_TYPE_DYNAMIC_FUNCTION):
					function = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.function;
					goto vm_opcode_local_call_dynamic;
				default:
					call_error = VM_ERROR_EXPECTED_CALLABLE_TYPE;
					goto opcode_local_call_skip_l;
			}
		case(VM_TYPE_POP_NODE): 
			switch ((enum vm_node_type) subtree->child[len - 1].type) {
				case(VM_TYPE_BYTECODE_FUNCTION):
					function = subtree->child[len - 1].value.function;
					if ((call_error = vm_node_clear(&subtree->child[len - 1], 0))) {
						VM_PANIC(call_error);
					}
					len--;
					goto vm_opcode_local_call;
				case(VM_TYPE_CORE_FUNCTION):
					function = subtree->child[len - 1].value.function;
					if ((call_error = vm_node_clear(&subtree->child[len - 1], 0))) {
						VM_PANIC(call_error);
					}
					len--;
					goto vm_opcode_local_call_core;
				case(VM_TYPE_EXTENDED_FUNCTION):
					function = subtree->child[len - 1].value.function;
					if ((call_error = vm_node_clear(&subtree->child[len - 1], 0))) {
						VM_PANIC(call_error);
					}
					len--;
					goto vm_opcode_local_call_library;
				case(VM_TYPE_DYNAMIC_FUNCTION):
					function = subtree->child[len - 1].value.function;
					if ((call_error = vm_node_clear(&subtree->child[len - 1], 0))) {
						VM_PANIC(call_error);
					}
					len--;
					goto vm_opcode_local_call_dynamic;
				default:
					call_error = VM_ERROR_EXPECTED_CALLABLE_TYPE;
					goto opcode_local_call_skip_l;
			}
		//Bytecode function
		case(VM_TYPE_BYTECODE_FUNCTION):
			function = VM_INSTRUCTION_GET(OPERANT)(uint32_t);
			vm_opcode_local_call:
			
			//Get label by useing uint32 as dpc_address
			function2.ld = function;
			nip = vm_get_label(function2);
			
			if (nip == NULL) {
				VM_PANIC(VM_ERROR_INVALID_JUMP_ADDRESS);
			}
			
			
			//Set values in subtree to null
			if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree != NULL) {
				subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->parent = NULL;
			}
			subtree->child[VM_INSTRUCTION_GET(NODE)].object_address = 0;
			
			//call and store error
			call_error = vm_call_node(nip, &subtree->child[VM_INSTRUCTION_GET(NODE)]);
			
			
			
			break;
		//Call core function
		case(VM_TYPE_CORE_FUNCTION):
			function = VM_INSTRUCTION_GET(OPERANT)(uint32_t);
			vm_opcode_local_call_core:
			
			if (function >= vmx_corelib_len) {
				VM_PANIC(VM_ERROR_INVALID_CORE_FUNCTION);
			}
			
			
			//Set values in subtree
			if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree != NULL) {
				subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->parent = NULL;
			}
			subtree->child[VM_INSTRUCTION_GET(NODE)].object_address = 0;
			
			//Call and store error
			call_error = vmx_corelib[function](&subtree->child[VM_INSTRUCTION_GET(NODE)]);
			
			
			
			break;
		//Call extended function
		case(VM_TYPE_EXTENDED_FUNCTION):
			function = VM_INSTRUCTION_GET(OPERANT)(uint32_t);
			vm_opcode_local_call_library:
			if (function >= vmx_extlib_len) {
				VM_PANIC(VM_ERROR_INVALID_EXTENDED_FUNCTION);
			}
			
			
			//Set values in subtree
			if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree != NULL) {
				subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->parent = NULL;
			}
			subtree->child[VM_INSTRUCTION_GET(NODE)].object_address = 0;
			
			//Call and store error
			call_error = vmx_extlib[function](&subtree->child[VM_INSTRUCTION_GET(NODE)]);
			
			
			
			break;
		//Call dynamic function
		case(VM_TYPE_DYNAMIC_FUNCTION):
			function = VM_INSTRUCTION_GET(OPERANT)(uint32_t);
			vm_opcode_local_call_dynamic:
			
			

			
			//Call and store error
			call_error = vmx_dynlib(function, &subtree->child[VM_INSTRUCTION_GET(NODE)]);
			
			
			break;
		default:
			call_error = VM_ERROR_EXPECTED_CALLABLE_TYPE;
			goto opcode_local_call_skip_l;
	}
	
	opcode_local_call_skip_l:
	

	//Reset values in subtree
	subtree->child[VM_INSTRUCTION_GET(NODE)].object_address = object_address2;
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].subtree != NULL) {
		subtree->child[VM_INSTRUCTION_GET(NODE)].subtree->parent = node;
	}

	//Check for returned error
	if (call_error) {
		VM_PANIC(VM_ERROR_FAILED_FUNCTION_CALL);
	}



	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_RETURN) {
	vm_error_t lerror;
	
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	
	//Store returned node node
	vm_node_t temp = subtree->child[VM_INSTRUCTION_GET(NODE)]; temp.object_address = 0;
	
	//Clear returned node
	subtree->child[VM_INSTRUCTION_GET(NODE)].value = VM_NULL_VALUE;
	subtree->child[VM_INSTRUCTION_GET(NODE)].type = VM_TYPE_NULL;
	subtree->child[VM_INSTRUCTION_GET(NODE)].subtree = NULL;
	
	//Clear and set parent node, to node returned
	if ((lerror = vm_node_clear(parent_call, 0))) {
		VM_PANIC(lerror);
	}
	;
	*parent_call = temp;
	
	
	VM_PANIC(VM_ERROR_EXITED);
	
	
	goto recontinue_l;
}

OPCODE_START(VM_OPCODE_ADD) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	double v = 0.0;
	
	//Get number from operant
	switch ((enum vm_node_type) VM_INSTRUCTION_GET(TYPE)) {
		//Indirect from node
		case(VM_TYPE_VALUE): {
			if (VM_INSTRUCTION_GET(OPERANT)(uint32_t) >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.number;
			break;
		}
		//Indirect from last node
		case(VM_TYPE_POP_NODE): {
			if (0 >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[len - 1].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[len - 1].value.number;

			vm_error_t lerror = 0;
			if ((lerror = vm_node_clear(&subtree->child[len - 1], 0))) {
				VM_PANIC(lerror);
			}
			len--;

			break;
		}
		//32-bit Literal
		case(VM_TYPE_INT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(int32_t); break;
		case(VM_TYPE_UINT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(uint32_t); break;
		case(VM_TYPE_FLOAT): v = (double) VM_INSTRUCTION_GET(OPERANT)(float); break;
		//64-bit literal
		case(VM_TYPE_DOUBLE): {
			//Get double by dpc_address
			double *dv = (double*) vm_get_data(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
			if (dv == NULL) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			v = *dv;
			break;
		}
		default: {
			VM_PANIC(VM_ERROR_UNKNOWN_OR_INVALID_TYPE);
		}
	}
	
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number += v;
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_SUB) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	double v = 0.0;
	
	//Get number from operant
	switch ((enum vm_node_type) VM_INSTRUCTION_GET(TYPE)) {
		//Indirect from node
		case(VM_TYPE_VALUE): {
			if (VM_INSTRUCTION_GET(OPERANT)(uint32_t) >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.number;
			break;
		}
		//Indirect from last node
		case(VM_TYPE_POP_NODE): {
			if (0 >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[len - 1].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[len - 1].value.number;

			vm_error_t lerror = 0;
			if ((lerror = vm_node_clear(&subtree->child[len - 1], 0))) {
				VM_PANIC(lerror);
			}
			len--;

			break;
		}
		//32-bit Literal
		case(VM_TYPE_INT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(int32_t); break;
		case(VM_TYPE_UINT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(uint32_t); break;
		case(VM_TYPE_FLOAT): v = (double) VM_INSTRUCTION_GET(OPERANT)(float); break;
		//64-bit literal
		case(VM_TYPE_DOUBLE): {
			//Get double by dpc_address
			double *dv = (double*) vm_get_data(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
			if (dv == NULL) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			v = *dv;
			break;
		}
		default: {
			VM_PANIC(VM_ERROR_UNKNOWN_OR_INVALID_TYPE);
		}
	}
	
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number -= v;
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_MUL) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	double v = 0.0;
	
	//Get number from operant
	switch ((enum vm_node_type) VM_INSTRUCTION_GET(TYPE)) {
		//Indirect from node
		case(VM_TYPE_VALUE): {
			if (VM_INSTRUCTION_GET(OPERANT)(uint32_t) >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.number;
			break;
		}
		//Indirect from last node
		case(VM_TYPE_POP_NODE): {
			if (0 >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[len - 1].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[len - 1].value.number;

			vm_error_t lerror = 0;
			if ((lerror = vm_node_clear(&subtree->child[len - 1], 0))) {
				VM_PANIC(lerror);
			}
			len--;

			break;
		}
		//32-bit Literal
		case(VM_TYPE_INT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(int32_t); break;
		case(VM_TYPE_UINT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(uint32_t); break;
		case(VM_TYPE_FLOAT): v = (double) VM_INSTRUCTION_GET(OPERANT)(float); break;
		//64-bit literal
		case(VM_TYPE_DOUBLE): {
			//Get double by dpc_address
			double *dv = (double*) vm_get_data(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
			if (dv == NULL) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			v = *dv;
			break;
		}
		default: {
			VM_PANIC(VM_ERROR_UNKNOWN_OR_INVALID_TYPE);
		}
	}
	
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number *= v;
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_DIV) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	double v = 0.0;
	
	//Get number from operant
	switch ((enum vm_node_type) VM_INSTRUCTION_GET(TYPE)) {
		//Indirect from node
		case(VM_TYPE_VALUE): {
			if (VM_INSTRUCTION_GET(OPERANT)(uint32_t) >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.number;
			break;
		}
		//Indirect from last node
		case(VM_TYPE_POP_NODE): {
			if (0 >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[len - 1].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[len - 1].value.number;

			vm_error_t lerror = 0;
			if ((lerror = vm_node_clear(&subtree->child[len - 1], 0))) {
				VM_PANIC(lerror);
			}
			len--;

			break;
		}
		//32-bit Literal
		case(VM_TYPE_INT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(int32_t); break;
		case(VM_TYPE_UINT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(uint32_t); break;
		case(VM_TYPE_FLOAT): v = (double) VM_INSTRUCTION_GET(OPERANT)(float); break;
		//64-bit literal
		case(VM_TYPE_DOUBLE): {
			//Get double by dpc_address
			double *dv = (double*) vm_get_data(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
			if (dv == NULL) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			v = *dv;
			break;
		}
		default: {
			VM_PANIC(VM_ERROR_UNKNOWN_OR_INVALID_TYPE);
		}
	}
	
	if (v == 0.0) {
		VM_PANIC(VM_ERROR_ARITHMETIC_ERROR);
	}
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number /= v;
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_MOD) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	double v = 0.0;
	
	//Get number from operant
	switch ((enum vm_node_type) VM_INSTRUCTION_GET(TYPE)) {
		//Indirect from node
		case(VM_TYPE_VALUE): {
			if (VM_INSTRUCTION_GET(OPERANT)(uint32_t) >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[VM_INSTRUCTION_GET(OPERANT)(uint32_t)].value.number;
			break;
		}
		//Indirect from last node
		case(VM_TYPE_POP_NODE): {
			if (0 >= len) {
				VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
			}
			if (subtree->child[len - 1].type != VM_TYPE_NUMBER) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			
			v = subtree->child[len - 1].value.number;

			vm_error_t lerror = 0;
			if ((lerror = vm_node_clear(&subtree->child[len - 1], 0))) {
				VM_PANIC(lerror);
			}
			len--;

			break;
		}
		//32-bit Literal
		case(VM_TYPE_INT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(int32_t); break;
		case(VM_TYPE_UINT32): v = (double) VM_INSTRUCTION_GET(OPERANT)(uint32_t); break;
		case(VM_TYPE_FLOAT): v = (double) VM_INSTRUCTION_GET(OPERANT)(float); break;
		//64-bit literal
		case(VM_TYPE_DOUBLE): {
			//Get double by dpc_address
			double *dv = (double*) vm_get_data(VM_INSTRUCTION_GET(OPERANT)(union vm_dpc_address));
			if (dv == NULL) {
				VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
			}
			v = *dv;
			break;
		}
		default: {
			VM_PANIC(VM_ERROR_UNKNOWN_OR_INVALID_TYPE);
		}
	}
	
	if (v == 0.0) {
		VM_PANIC(VM_ERROR_ARITHMETIC_ERROR);
	}
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number = fmod(subtree->child[VM_INSTRUCTION_GET(NODE)].value.number, v);
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_NEG) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number *= -1;
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_FLOOR) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number = floor(subtree->child[VM_INSTRUCTION_GET(NODE)].value.number);
	
	
	goto recontinue_l;
}
OPCODE_START(VM_OPCODE_CEIL) {
	if (VM_INSTRUCTION_GET(NODE) >= len) {
		VM_PANIC(VM_ERROR_OUT_OF_RANGE_NODE);
	}
	
	//Check for type
	if (subtree->child[VM_INSTRUCTION_GET(NODE)].type != VM_TYPE_NUMBER) {
		VM_PANIC(VM_ERROR_EXPECTED_NUMBER);
	}
	
	
	subtree->child[VM_INSTRUCTION_GET(NODE)].value.number = ceil(subtree->child[VM_INSTRUCTION_GET(NODE)].value.number);
	
	
	goto recontinue_l;
}




#ifdef VM_GOTO_MODE
	opcode_undef_l: VM_PANIC(VM_ERROR_INVALID_OPCODE);
#else
	default: VM_PANIC(VM_ERROR_INVALID_OPCODE);
#endif




#endif//TREEVM_OPCODES_H_