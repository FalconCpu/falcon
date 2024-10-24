#include <stdio.h>
#include "fpl.h"

// ====================================================
//                    Constructor
// ====================================================

AST_list new_list() {
    AST_list ret = new(struct AST_list);
    ret->num_elements = 0;
    ret->alloc_elements = 8;
    ret->elements = new_array(AST, ret->alloc_elements);
    return ret;
}

// ====================================================
//                    list_add
// ====================================================

void list_add(AST_list l, AST e) {
    if (l->num_elements==l->alloc_elements) {
        l->alloc_elements *= 2;
        resize_array(l->elements, AST, l->alloc_elements);
    }
    l->elements[l->num_elements++] = e;
}

// ====================================================
//                    list_get
// ====================================================

AST list_get(AST_list l, int index) {
    if (index<0 || index>=l->num_elements)
        fatal("Internal Error: Index %d out of range 0..%d", index, l->num_elements-1);
    return l->elements[index];
}
