

static Hash_Element* hash_add(String name, TokenKind kind) {
    if (hash_count*4 > hash_size*3) {
        Hash_Element *old_table = hash_table;
        hash_size *= 2;
        hash_table = new_array(Hash_Element, hash_size);
        hash_count = 0;
        for(int k=0; k<hash_size/2; k++)
            if (old_table[k].name)
                hash_add(old_table[k].name, old_table[k].kind);
        free(old_table);
    }

    int slot = hash_function(name);
    while(hash_table[slot].name)
        slot = (slot+1) & (hash_size-1);
    hash_table[slot].name = name;
    hash_table[slot].kind = kind;
    return &hash_table[slot];
}