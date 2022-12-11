class cache_node:
    prev = None
    next = None
    value = None

    def __init__(self, value = None, prev = None, next = None):
        self.value = value
        self.prev = prev
        self.next = next

class cache_list:
    begin = None
    end = None

    def push_back(self, elem: int):
        if self.begin is None and self.end is None:
            self.begin = self.end = cache_node(elem)
        elif self.begin is self.end:
            self.begin.next = cache_node(elem, self.begin, None)
            self.end = self.begin.next
        else:
            self.end.next = cache_node(elem, self.end, None)
            self.end = self.end.next


    def pop_front(self):
        node = self.begin
        if self.begin.value == self.end.value:
            self.begin = None
            self.end = None
        elif self.begin.next == self.end:
            self.begin = self.end
            self.begin.next = self.end.prev = None
        else:
            self.begin = self.begin.next
            self.begin.prev = None

        return node

    def pop_back(self):
        node = self.end
        if self.begin.value == self.end.value:
            self.begin = None
            self.end = None
        elif self.begin.next == self.end:
            self.end = self.begin
            self.begin.next = self.end.prev = None
        else:
            self.end = self.end.prev
            self.end.next = None
        return node

    def delete_node(self, node):
        if node == self.begin:
            self.pop_front()
        elif node == self.end:
            self.pop_back()
        else:
            if node.prev:
                node.prev.next = node.next
            if node.next:
                node.next.prev = node.prev

class cache:
    total_size = 0
    current_size = 0
    cache_indexes = cache_list()
    cache_memory = []
    def __init__(self, size: int, number_of_quantums: int):
        self.total_size = size
        self.current_size = 0
        self.cache_memory = [None for i in range(number_of_quantums)]

    def add(self, elem: int) -> int:
        return_value = -1
        if self.current_size < self.total_size:
            self.current_size += 1
        else:
            node = self.cache_indexes.pop_front()
            return_value = node.value
            self.cache_memory[return_value] = None
        self.cache_indexes.push_back(elem)
        self.cache_memory[elem] = self.cache_indexes.end
        return return_value

    def remove(self, elem: int):
        if not (self.cache_memory[elem] is None):
            self.cache_indexes.delete_node(self.cache_memory[elem])
            self.cache_memory[elem] = None
            self.current_size -= 1

    def is_contain(self, elem: int) -> bool:
        return not self.cache_memory[elem] is None

    def update(self, elem: int):
        self.cache_indexes.delete_node(self.cache_memory[elem])
        self.cache_indexes.push_back(elem)
        self.cache_memory[elem] = self.cache_indexes.end

    def print(self):
        cur = self.cache_indexes.begin
        while not (cur is None):
            print(cur.value, end=" ")
            cur = cur.next
        print()
