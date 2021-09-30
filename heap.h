#ifndef TREEVM_MAIN_SOURCE_FILE
	#error This file must not be included
#endif

#ifndef TREEVM_HEAP_H_
#define TREEVM_HEAP_H_ 1


#include <stdlib.h>
#include <inttypes.h>


#undef VMX_LHCAT
#undef VM_LHAREA
#undef VM_LHAREA_COUNT
#ifndef EXPLICIT_LHCAT
	#define VMX_LHCAT 8
#endif
#define VM_LHAREA (1<<VMX_LHCAT)
#define VM_LHAREA_COUNT (32 - VMX_LHCAT)



struct vm_heap {
	struct vm_heap_node {
		vm_node_t node;
		
		uint32_t phase;
		
		uint32_t references;				/* Number of references */
		
		int loaded;			/* Is object loaded */
		
		
		struct vm_heap_node *next;
	} *node;
	
	uint32_t size;		/* Max 2^31, 0x80000000 */
	
	
	
	unsigned int area_len;
	struct vm_heap_area {
		struct vm_heap_node *free;
		uint32_t len;
	} area[32];
};



#define VM_HEAP_MAX_SIZE2 (VM_SIZE_MAX/sizeof(struct vm_heap_node))		//the maximum the size_t type allows
static const uint32_t VM_HEAP_MAX_SIZE = (0x80000000 > VM_HEAP_MAX_SIZE2) ? ((uint32_t) VM_HEAP_MAX_SIZE2) : 0x80000000;
#undef VM_HEAP_MAX_SIZE2



static struct vm_heap vm_heap = { .size = 0, };		//Set size to 0, so that heap functions on an unitialized heap dont work





static unsigned int vm_heap_get_area(uint32_t node_index) {
	unsigned int index = 0;
	node_index /= VM_LHAREA/2;
	for (; node_index > 1; node_index>>=1, index++);
	return(index);
}


//Initialize heap manager
static vm_error_t vm_heap_init() {
	//Allocate nodes
	vm_heap.node = (struct vm_heap_node*) SMALLOC(VM_LHAREA * sizeof(struct vm_heap_node));
	if (vm_heap.node == NULL)
		return(VM_ERROR_INIT_OUT_OF_MEMORY);
	
	vm_error_t error = 0;
	if ((error = vmx_provide_heap_size(VM_LHAREA))) {
		SFREE(vm_heap.node);
		return(error);
	}

	vm_heap.size = VM_LHAREA;


	//Initialize Areas
	for (int i = 0; i < 32; i++) {
		vm_heap.area[i].free = NULL;
		vm_heap.area[i].len = 0;
	}
	vm_heap.area_len = 1;
	
	//Initialize nodes
	for (uint32_t i = 1; i < VM_LHAREA; i++) {
		//Initialize values
		vm_heap.node[i].node = VM_NODE_INITIALIZER(VM_TYPE_NULL);
		vm_heap.node[i].node.object_address = i;
		
		if (vm_heap.node[i].phase == 0) {
			vm_heap.node[i].phase++;
		}
		
		//Set next pointer for free list
		vm_heap.node[i].next = vm_heap.area[0].free;
		vm_heap.area[0].free = &vm_heap.node[i];
	}

	
	return(0);
}


//Destroy heap Manager
static void vm_heap_destroy() {
	//clear nodes
	for (uint32_t i = 1; i < vm_heap.size; i++) {
		if (vm_heap.node[i].next != NULL)
			continue;
		vm_node_clear(&vm_heap.node[i].node, 0);
	}

	vmx_provide_heap_size(0);

	
	//Free nodes
	SFREE(vm_heap.node);
}




//Add a new heap area
static vm_error_t vm_heap_expand() {
	const uint32_t lsize = vm_heap.size; const uint32_t lsize2 = lsize * 2; 
	
	if (vm_heap.area_len >= VM_LHAREA_COUNT || lsize2 > VM_HEAP_MAX_SIZE) {
		return(VM_ERROR_HEAP_REACHED_MAX_SIZE);
	}
	
	
	void *new_ptr = SREALLOC(vm_heap.node, lsize2 * sizeof(struct vm_heap_node));
	if (new_ptr == NULL) {
		return(VM_ERROR_OUT_OF_MEMORY);
	}

	vm_error_t error = 0;
	if ((error = vmx_provide_heap_size(lsize2))) {
		return(error);
	}

	vm_heap.size = lsize2;
	vm_heap.area_len++;
	

	//Initialize new Node
	vm_heap.area[vm_heap.area_len].len = 0;
	vm_heap.area[vm_heap.area_len].free = NULL;
	
	
	//Initialize nodes
	for (uint32_t i = lsize; i < lsize2; i++) {
		//Initialize values
		vm_heap.node[i].node = VM_NODE_INITIALIZER(VM_TYPE_NULL);
		vm_heap.node[i].node.object_address = i;
		
		if (vm_heap.node[i].phase == 0) {
			vm_heap.node[i].phase++;
		}
		
		//Set next pointer for free list
		vm_heap.node[i].next = vm_heap.area[vm_heap.area_len].free;
		vm_heap.area[vm_heap.area_len].free = &vm_heap.node[i];
	}
	
	
	return(0);
}


//Remove last heap area
static void vm_heap_shrink() {
	if (vm_heap.area_len <= 1) {
		return;
	}
	
	const uint32_t lsize = vm_heap.size / 2;
	void *new_ptr = SREALLOC(vm_heap.node, lsize * sizeof(struct vm_heap_node));
	if (new_ptr != NULL) {
		vm_heap.node = new_ptr;
	}
	
	vmx_provide_heap_size(lsize);
	vm_heap.size = lsize;
	vm_heap.area_len--;
}




//Allocate object on the heap, return 0 on error
vm_error_t vm_heap_alloc(vm_node_t * const restrict node) {
	if (running == 0) {
		return(VM_ERROR_VM_NOT_RUNNING);
	}
	
	//Find first free heap area
	unsigned int area = 0;
	for (area = 0; area < vm_heap.area_len; area++) {
		if (vm_heap.area[area].free == NULL)
			continue;
	}
	
	if (area == vm_heap.area_len) {
		if (vm_heap_expand())
			return(VM_ERROR_OUT_OF_MEMORY);
	}
	struct vm_heap_node * const object = vm_heap.area[area].free;
	
	
	const uint32_t address = (uint32_t) (object - vm_heap.node);
	
	
	//Initialize Object
	vm_error_t error = 0;
	if ((error = vm_node_clear_value(&object->node))) {
		return(error);
	}
	object->node = VM_NODE_INITIALIZER(VM_TYPE_NULL);
	object->node.object_address = address;
	object->loaded = 0;
	if (object->phase == 0) {
		object->phase++;
	}
	object->references = 0;

	
	
	
	
	//Update free list
	vm_heap.area[area].free = vm_heap.area[area].free->next;
	object->next = NULL;
	vm_heap.area[area].len++;
	
	
	//Initialize reference
	node->type = VM_TYPE_OBJECT_REFERENCE;
	node->value.object.address = address;
	node->value.object.phase = object->phase;
	
	
	
	return(0);
}



//Free object on heap
vm_error_t vm_heap_free(vm_node_t * const restrict node) {
	if (node->type != VM_TYPE_OBJECT_REFERENCE) {
		return(VM_ERROR_EXPECTED_OBJECT_REFERENCE);
	}
	
	if (node->value.object.address == 0 || node->value.object.address >= vm_heap.size || vm_heap.node[node->value.object.address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	struct vm_heap_node * const restrict object = &vm_heap.node[node->value.object.address];
	
	if (object->loaded) {
		return(VM_ERROR_LOADED_OBJECT);
	}
	
	if (/*reference.phase != 0 && */node->value.object.phase != object->phase) {
		return(VM_ERROR_OBJECT_OUT_OF_PHASE);
	}
	
	
	//Deinitialize Object
	object->phase++;
	if (object->phase == 0) {
		object->phase++;
	}
	
	
	vm_error_t clean_error = vm_node_clear(&object->node, 0);
	
	
	//Update area and free list
	unsigned int area = vm_heap_get_area(node->value.object.address);
	object->next = vm_heap.area[area].free;
	vm_heap.area[area].free = object;
	vm_heap.area[area].len--;
	if (vm_heap.area[area].len == 0 && area == (vm_heap.area_len - 1)) {
		vm_heap_shrink();
	}
	
	//Clear reference
	node->value.object.phase = node->value.object.address;
	node->value.object.address = 0;


	return(clean_error);
}




//Create reference to node
vm_error_t vm_heap_create_reference(vm_node_t * const restrict node) {
	if (node->type != VM_TYPE_OBJECT_REFERENCE) {
		return(VM_ERROR_EXPECTED_OBJECT_REFERENCE);
	}
	
	if (node->value.object.address == 0 || node->value.object.address >= vm_heap.size || vm_heap.node[node->value.object.address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	struct vm_heap_node * const restrict object = &vm_heap.node[node->value.object.address];
	
	if (object->phase != node->value.object.phase) {
		return(VM_ERROR_OBJECT_OUT_OF_PHASE);
	}
	
	
	if (object->next == NULL)
		object->references++;

	
	return(0);
}


//Create reference to node
vm_error_t vm_heap_remove_reference(vm_node_t * const restrict node) {
	if (node->type != VM_TYPE_OBJECT_REFERENCE) {
		return(VM_ERROR_EXPECTED_OBJECT_REFERENCE);
	}
	
	if (node->value.object.address == 0 || node->value.object.address >= vm_heap.size || vm_heap.node[node->value.object.address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	struct vm_heap_node * const restrict object = &vm_heap.node[node->value.object.address];
	
	if (node->value.object.phase != object->phase) {
		return(VM_ERROR_OBJECT_OUT_OF_PHASE);
	}
	
	if (object->next == NULL)
		object->references--;
	
	//Clear reference
	node->value.object.phase = node->value.object.address;
	node->value.object.address = 0;

	//Free object if no reference remaining
	if (object->references == 0) {
		return(vm_heap_free(node));
	}
	

	return(0);
}


//Update reference to node
vm_error_t vm_heap_load_object(vm_node_t * const restrict node) {
	if (node->type != VM_TYPE_OBJECT_REFERENCE) {
		return(VM_ERROR_EXPECTED_OBJECT_REFERENCE);
	}
	
	if (node->value.object.address == 0 || node->value.object.address >= vm_heap.size || vm_heap.node[node->value.object.address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	struct vm_heap_node * const restrict object = &vm_heap.node[node->value.object.address];
	
	if (node->value.object.phase != object->phase) {
		return(VM_ERROR_OBJECT_OUT_OF_PHASE);
	}
	
	if (object->loaded) {
		return(VM_ERROR_LOADED_OBJECT);
	}
	
	
	vm_node_t tnode = *node;
	*node = object->node;
	object->node = tnode;
	
	object->loaded = 1;
	
	return(0);
}


//Remove reference to node
vm_error_t vm_heap_unload_object(vm_node_t * const restrict node) {
	if (node->object_address == 0 || node->object_address >= vm_heap.size || vm_heap.node[node->object_address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	struct vm_heap_node * const restrict object = &vm_heap.node[node->object_address];
	
	if (!object->loaded) {
		return(VM_ERROR_UNLOADED_OBJECT);
	}
	
	
	vm_node_t tnode = object->node;
	object->node = *node;
	*node = tnode;
	
	object->loaded = 0;
	
	
	return(0);
}




vm_error_t vm_heap_verify(vm_node_t * const restrict node) {
	if (node->type != VM_TYPE_OBJECT_REFERENCE) {
		return(VM_ERROR_EXPECTED_OBJECT_REFERENCE);
	}
	
	if (node->value.object.address == 0 || node->value.object.address >= vm_heap.size || vm_heap.node[node->value.object.address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	if (node->value.object.phase != vm_heap.node[node->value.object.address].phase) {
		return(VM_ERROR_OBJECT_OUT_OF_PHASE);
	}
	
	if (vm_heap.node[node->value.object.address].loaded) {
		return(VM_ERROR_LOADED_OBJECT);
	}
	
	
	return(0);
}


vm_error_t vm_heap_verify_open(vm_node_t * const restrict node) {
	if (node->object_address == 0 || node->object_address >= vm_heap.size || vm_heap.node[node->object_address].next != NULL) {
		return(VM_ERROR_NO_SUCH_OBJECT);
	}
	
	
	if (!vm_heap.node[node->object_address].loaded) {
		return(VM_ERROR_UNLOADED_OBJECT);
	}
	
	
	
	return(0);
}


#endif//TREEVM_HEAP_H_
