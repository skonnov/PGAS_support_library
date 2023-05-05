#include <mpi.h>
#include "common.h"

struct cache_node {
    int value;
    cache_node* next, *prev;
};

class cache_list {
    cache_node* begin = nullptr, *end = nullptr;
    int size = 0;
public:
    void push_back(cache_node* node);
    cache_node* pop_front();
    cache_node* pop_back();
    void delete_node(cache_node* node);
    bool empty();
    cache_node* get_begin();
};
