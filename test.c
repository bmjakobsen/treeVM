#include <stdio.h>
#include "./include.h"

vm_core_t vmx_core = {0};

int vmx_interrupt(const uint16_t int_id) { return(0); }
void vmx_return_cleanup(vm_node_t * const node, const vm_error_t error) { ; }

void vmx_interpreter_lock() { ; }
void vmx_interpreter_unlock() { ; }

vm_error_t (* const vmx_corelib[])(vm_node_t *) = { NULL, };
const uint32_t vmx_corelib_len = 0;

vm_error_t (* * vmx_extlib)(vm_node_t *) = NULL;
uint32_t vmx_extlib_len = 0;

vm_error_t vmx_dynlib(const uint32_t fn_id, vm_node_t * const node) { return(1); }
int vmx_dynlib_test(const uint32_t fn_id) { return(1); }

vm_error_t vmx_provide_heap_size(uint32_t size) { return(0); }


int main() {
	printf("%.8f\n\n", VM_MAX_PRECISE_DOUBLE);
	vm_print_information(0);
	getchar();
}
