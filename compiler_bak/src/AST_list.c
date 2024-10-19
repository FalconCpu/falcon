#include "fpl.h"

// ---------------------------------------------------------------------------------
//                         ASTlist
// ---------------------------------------------------------------------------------


AST_list new_AST_list() {
    AST_list ret = new(struct AST_list);
    return ret;
}

void    AST_list_add(AST_list list, AST item) {
    if (list->count==list->alloc) {
        if (list->alloc==0) {
            list->alloc = 4;
            list->items = new_array(AST, list->alloc);
        } else {
            list->alloc *= 2;
            resize_array(list->items, AST, list->alloc);
        }
    }
    list->items[list->count++] = item;
}

AST  AST_list_get(AST_list list, int index){
    if (list==0)
        fatal("Null pointer in ASTlist_get");
    if (index<0 || index>list->count)
        fatal("Index %d out of range [0..%d] in ASTlist_get", index, list->count);
    if (index==list->count)
        return 0;
    return list->items[index];
}