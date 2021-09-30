#include <stdio.h>
#include "./include.h"



/*
Simple test file,
declares the required variables and functions for use with the vm
*/


vm_core_t vmx_core = {0};

//Only required for use with an interpreter
void vmx_interpreter_lock() { ; }
void vmx_interpreter_unlock() { ; }

int vmx_interrupt(const uint16_t int_id) { return(0); }
void vmx_return_cleanup(vm_node_t * const node, const vm_error_t error) { ; }

vm_error_t vmx_provide_heap_size(uint32_t size) { return(0); }

//Core library
vm_error_t (* const vmx_corelib[])(vm_node_t *) = { NULL, };
const uint32_t vmx_corelib_len = 0;

//Extended library
vm_error_t (* * vmx_extlib)(vm_node_t *) = NULL;
uint32_t vmx_extlib_len = 0;

//Dynamic library
vm_error_t vmx_dynlib(const uint32_t fn_id, vm_node_t * const node) { return(VM_ERROR_INVALID_DYNAMIC_FUNCTION); }
int vmx_dynlib_test(const uint32_t fn_id) { return(1); }



//Only required when -DEXPLICIT_LHCAT is set
const unsigned int VMX_LHCAT = 0;


int main() {
	printf("%.8f\n\n", VM_MAX_PRECISE_DOUBLE);
	int r = vm_get_build_information(0);

	printf("\nBuild invalid: %d\n", r);
	getchar();
}
