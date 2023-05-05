#include "cache_list.h"

void cache_list::push_back(cache_node* node) {
    CHECK(node != nullptr, STATUS_ERR_NULLPTR);
    if (begin == nullptr && end == nullptr) {
        begin = end = node;
        node->next = node->prev = nullptr;
    } else if (begin == end) {
        begin->next = node;
        node->prev = begin;
        node->next = nullptr;
        end = node;
    } else {
        end->next = node;
        node->prev = end;
        node->next = nullptr;
        end = node;
    }
    ++size;
}

cache_node* cache_list::pop_front() {
    CHECK(begin != nullptr, STATUS_ERR_NULLPTR);
    cache_node* return_value = begin;
    if (begin == end) {
        begin = end = nullptr;
    } else if (begin->next == end) {
        begin = end;
        begin->prev = begin->next = end->next = end->prev = nullptr;
    } else {
        CHECK(begin->next != nullptr, STATUS_ERR_NULLPTR);
        begin = begin->next;
        begin->prev = nullptr;
    }
    --size;
    return return_value;
}

cache_node* cache_list::pop_back() {
    CHECK(end != nullptr, STATUS_ERR_NULLPTR);
    cache_node* return_value = end;
    if (begin == end) {
        begin = end = nullptr;
    } else if (begin->next == end) {
        begin = end;
        begin->prev = begin->next = end->next = end->prev = nullptr;
    } else {
        CHECK(end->prev != nullptr, STATUS_ERR_NULLPTR);
        end = end->prev;
        end->next = nullptr;
    }
    --size;
    return return_value;
}

void cache_list::delete_node(cache_node* node) {
    CHECK(node != nullptr, STATUS_ERR_NULLPTR);
    CHECK(begin!= nullptr, STATUS_ERR_NULLPTR);
    cache_node* next_node = node->next;
    if (node == begin) {
        pop_front();
    } else if (node == end) {
        pop_back();
    } else {
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
    }
    node->prev = node->next = nullptr;
}

bool cache_list::empty() {
    return begin == end && begin == nullptr;
}

cache_node* cache_list::get_begin() {
    return begin;
}
