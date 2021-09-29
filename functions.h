#ifndef TREEVM_MAIN_SOURCE_FILE
	#error This file must not be included
#endif

#ifndef TREEVM_FUNCTIONS_H_
	#define TREEVM_FUNCTIONS_H_ 1
	
	
	
	static inline vm_data_unit_t *vm_get_data(union vm_dpc_address address);
	static inline vm_instruction_t *vm_get_label(union vm_dpc_address address);
	
	
	#ifdef VM_INTERPRETER
		static inline vm_data_unit_t *vm_get_data(union vm_dpc_address address) {
			vmx_interpreter_lock();
			
			
			if (address.it[0] >= vmx_core.it.data_len) {
				vmx_interpreter_unlock();
				return(NULL);
			}
			if (address.it[0] >= vmx_core.it.data[address.it[0]].len) {
				vmx_interpreter_unlock();
				return(NULL);
			}
			vm_data_unit_t *t = &vmx_core.it.data[address.it[0]].p[address.it[1]];
			
			
			vmx_interpreter_unlock();
			
			return(t);
		}
		static inline vm_instruction_t *vm_get_label(union vm_dpc_address address) {
			vmx_interpreter_lock();
			
			
			if (address.it[0] >= vmx_core.it.program_len) {
				vmx_interpreter_unlock();
				return(NULL);
			}
			if (address.it[0] >= vmx_core.it.program[address.it[0]].len) {
				vmx_interpreter_unlock();
				return(NULL);
			}
			vm_instruction_t *t = (vm_instruction_t *) &vmx_core.it.program[address.it[0]].p[address.it[1]];
			
			
			vmx_interpreter_unlock();
			
			return(t);
		}
	#else
		static inline vm_data_unit_t *vm_get_data(union vm_dpc_address address) {
			if (address.ld >= vmx_core.ld.data_len) {
				return(NULL);
			}
			
			return(&vmx_core.ld.data[address.ld]);
		}
		static inline vm_instruction_t *vm_get_label(union vm_dpc_address address) {
			if (address.ld >= vmx_core.ld.program_len) {
				return(NULL);
			}
			
			return((vm_instruction_t *) &vmx_core.ld.program[address.ld]);
		}
	#endif
	
	
	
	
	
	
	vm_error_t vm_node_copy_value(vm_node_t * const restrict src, vm_node_t * const restrict trg) {
		{
			uint32_t object_address = trg->object_address;
			*trg = *src;
			trg->object_address = object_address;
		}

		
		if (trg->type == VM_TYPE_STRING) {
			trg->value.string = SMALLOC(sizeof(vm_string_t) + (sizeof(char) * (src->value.string->size + 1)))
			if (trg->value.string == NULL)
				return(VM_ERROR_OUT_OF_MEMORY);
				
			memcpy(trg->value.string, src->value.string, sizeof(vm_string_t) + (sizeof(char) * (src->value.string->size + 1)));
		} else if (trg->type == VM_TYPE_OBJECT_REFERENCE) {
			vm_error_t lerror;
			if ((lerror = vm_heap_create_reference(trg))) {
				*trg = VM_NODE_INITIALIZER(VM_TYPE_NULL);
				return(lerror);
			}
		} else if (trg->type == VM_TYPE_CUSTOM_TYPE) {
			trg->value.custom_type = trg->value.custom_type->copy(trg->value.custom_type);
			if (trg->value.custom_type == NULL) {
				return(VM_ERROR_FAILED_TO_COPY_CUSTOM_TYPE);
			}
		}
		
		return(0);
	}
	
	vm_error_t vm_node_copy(vm_node_t * const restrict src, vm_node_t * const restrict trg) {
		vm_error_t lerror;
		if ((lerror = vm_node_copy_value(src, trg))) {
			return(lerror);
		}
		
		
		if (src->subtree != NULL) {
			trg->subtree = SMALLOC(sizeof(struct vm_node_subtree) + (sizeof(vm_node_t) * src->subtree->size));
			if (trg->subtree == NULL) {
				return(VM_ERROR_OUT_OF_MEMORY);
			}
			*trg->subtree = *src->subtree;
			
			for (uint32_t i = 0; i < trg->subtree->len; i++) {
				if ((lerror = vm_node_copy(&src->subtree->child[i], &trg->subtree->child[i]))) {
					for (uint32_t j = 0; j < i; j++) {
						if (trg->subtree->child[j].type == VM_TYPE_OBJECT_REFERENCE) {	//Clear newly created object references
							vm_heap_remove_reference(&trg->subtree->child[j]);
						} else if (trg->subtree->child[j].type == VM_TYPE_STRING) {		//Free newly created strings
							SFREE(trg->subtree->child[j].value.string);
						}
						trg->subtree->child[j] = VM_NODE_INITIALIZER(VM_TYPE_NULL);
					}
					SFREE(trg->subtree);
					return(lerror);
				}
			}
		}
		
		return(0);
	}
	
	
	
	
	vm_error_t vm_node_clear_value(vm_node_t * const restrict node) {
		vm_error_t error = VM_ERROR_NONE;

		if (node->type == VM_TYPE_STRING) {		//Free string
			SFREE(node->value.string);
		} else if (node->type == VM_TYPE_OBJECT_REFERENCE) {			
			error = vm_heap_remove_reference(node); 	//Remove object reference
		} else if (node->type == VM_TYPE_CUSTOM_TYPE) {			//Free custom type
			if (node->value.custom_type != NULL) {
				node->value.custom_type->free(node->value.custom_type);
				node->value.custom_type = NULL;
			}
		}
		
		//Clear value and type
		node->value = VM_NULL_VALUE;
		node->type = VM_TYPE_NULL;
		
		return(error);
	}
	
	vm_error_t vm_node_clear_children(vm_node_t * const restrict node) {
		vm_error_t error = VM_ERROR_NONE;

		//Clear children of the subtree
		if (node->subtree != NULL) {
			for (uint32_t i = 0; i < node->subtree->len; i++) {
				error = vm_node_clear(&node->subtree->child[i], 0);
			}
			SFREE(node->subtree);
		}
		
		return(error);
	}
	
	vm_error_t vm_node_clear(vm_node_t * const restrict node, const int preserve_open_object) {
		vm_error_t error = VM_ERROR_NONE;
		vm_error_t lerror = VM_ERROR_NONE;

		do {	//Looping because there can be open nested objects on the same node
			//Clear the value of the node
			if ((lerror = vm_node_clear_value(node))) {
				error = lerror;
			}
	
			//Clear the subtree of the node
			if ((lerror = vm_node_clear_children(node))) {
				error = lerror;
			}
			
			//Skip unloading of open object and escape the loop
			if (preserve_open_object) {
				break;
			}
			
			//Unload open object
			if (node->object_address == 0) {
				if ((lerror = vm_heap_unload_object(node))) {
					error = lerror;
				}	
			}
		} while (node->object_address != 0);

		
		
		return(error);
	}
	
	
	
	static vm_error_t vm_node_set(const vm_type_t type, struct vm_node_subtree * const subtree, struct vm_node * const node, uint32_t * const arg) {
		vm_error_t lerror = 0;
		vm_string_t * restrict lstring = NULL;
		vm_string_t * restrict string = NULL;

		if ((lerror = vm_node_clear_value(node))) {
			return(lerror);
		}
		
		switch ((enum vm_node_type) type) {
			case(VM_TYPE_NULL):
				node->value = VM_NULL_VALUE;
				node->type = VM_TYPE_NULL;
				break;
			case(VM_TYPE_INT32):
				node->value.number = *(int32_t*) arg;
				node->type = VM_TYPE_NUMBER;
				break;
			case(VM_TYPE_UINT32):
				node->value.number = *arg;
				node->type = VM_TYPE_NUMBER;
				break;
			case(VM_TYPE_FLOAT):
				node->value.number = *(float*) arg;
				node->type = VM_TYPE_NUMBER;
				break;
			case(VM_TYPE_DOUBLE):
				node->value.number = *(double*) vm_get_data(*(union vm_dpc_address*) arg);
				node->type = VM_TYPE_NUMBER;
				break;
			case(VM_TYPE_STRING):
				lstring = (vm_string_t*) vm_get_data(*(union vm_dpc_address*) arg);
				lstring->len = (uint32_t) strnlen(lstring->c, lstring->len);
				for(lstring->size = 1; lstring->size < lstring->len; lstring->size *= 2);
				
				string = SMALLOC(sizeof(vm_string_t) + (sizeof(char) * (lstring->size + 1)));
				if (string == NULL)
					return(VM_ERROR_OUT_OF_MEMORY);
				
				string->len = lstring->len;
				string->size = lstring->size;
				memset(string->c, 0, sizeof(char) * (lstring->size + 1));
				memcpy(string->c, lstring->c, sizeof(char) * string->len);
				
				node->value.string = string;
				node->type = VM_TYPE_STRING;
				break;
			case(VM_TYPE_VALUE):
				if (*arg >= subtree->len) {
					return(VM_ERROR_OUT_OF_RANGE_NODE);
				}
				
				if ((lerror = vm_node_copy_value(&subtree->child[*arg], node))) {
					return(lerror);
				}
				break;
			case(VM_TYPE_NODE):
				if (*arg >= subtree->len) {
					return(VM_ERROR_OUT_OF_RANGE_NODE);
				}

				//Clear node children
				if ((lerror = vm_node_clear_children(node))) {
					return(lerror);
				}

				//Copy node
				if ((lerror = vm_node_copy(&subtree->child[*arg], node))) {
					return(lerror);
				}
				break;
			case(VM_TYPE_CUT_NODE):
				if (*arg >= subtree->len) {
					return(VM_ERROR_OUT_OF_RANGE_NODE);
				}

				//Clear node children
				if ((lerror = vm_node_clear_children(node))) {
					return(lerror);
				}

				node->value = subtree->child[*arg].value;
				node->type = subtree->child[*arg].type;
				node->subtree = subtree->child[*arg].subtree;

				subtree->child[*arg].value = VM_NULL_VALUE;
				subtree->child[*arg].type = VM_TYPE_NULL;
				subtree->child[*arg].subtree = NULL;
				break;
			case(VM_TYPE_POP_NODE):
				if (subtree->len > 0) {
					return(VM_ERROR_OUT_OF_RANGE_NODE);
				}

				//Clear node children
				if ((lerror = vm_node_clear_children(node))) {
					return(lerror);
				}

				//Get node value
				node->value = subtree->child[subtree->len - 1].value;
				node->type = subtree->child[subtree->len - 1].type;
				node->subtree = subtree->child[subtree->len - 1].subtree;

				subtree->child[subtree->len - 1].value = VM_NULL_VALUE;
				subtree->child[subtree->len - 1].type = VM_TYPE_NULL;
				subtree->child[subtree->len - 1].subtree = NULL;

				//Clear original node
				if ((lerror = vm_node_clear(&subtree->child[subtree->len - 1], 0))) {
					return(lerror);
				}
				subtree->len--;

				break;
			case(VM_TYPE_BYTECODE_FUNCTION):
				if (vm_get_label(*(union vm_dpc_address*) arg) == NULL) {
					return(VM_ERROR_INVALID_JUMP_ADDRESS);
				}
				node->value.function = *arg;
				node->type = VM_TYPE_BYTECODE_FUNCTION;
				break;
			case(VM_TYPE_CORE_FUNCTION):
				if (*arg >= vmx_corelib_len) {
					return(VM_ERROR_INVALID_CORE_FUNCTION);
				}
				node->value.function = *arg;
				node->type = VM_TYPE_CORE_FUNCTION;
				break;
			case(VM_TYPE_EXTENDED_FUNCTION):
				if (*arg > vmx_extlib_len) {
					return(VM_ERROR_INVALID_EXTENDED_FUNCTION);
				}
				node->value.function = *arg;
				node->type = VM_TYPE_EXTENDED_FUNCTION;
				break;
			case(VM_TYPE_DYNAMIC_FUNCTION):
				if (vmx_dynlib_test(*arg)) {
					return(VM_ERROR_INVALID_DYNAMIC_FUNCTION);
				}
				
				node->value.function = *arg;
				node->type = VM_TYPE_DYNAMIC_FUNCTION;
				break;
			default: {
				return(VM_ERROR_UNKNOWN_OR_INVALID_TYPE);
			}
		}
		
		return(0);
	}
	
	
	
#endif//TREEVM_FUNCTIONS_H_
