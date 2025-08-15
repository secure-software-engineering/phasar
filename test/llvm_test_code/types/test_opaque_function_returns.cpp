struct Node {
  int data;
  Node *next;
};

// Functions returning pointers - all return 'ptr' in IR
Node *create_node(int value) { return new Node{value, nullptr}; }

Node *find_last(Node *head) {
  while (head && head->next) {
    head = head->next; // Load -> ptr
  }
  return head; // Return ptr
}

int *get_data_ptr(Node *node) {
  return &node->data; // GEP -> ptr
}

void **get_generic_ptr() {
  static void *ptr = nullptr;
  return &ptr; // Address of global -> ptr
}

void test_function_returns() {
  Node *n1 = create_node(42);         // Call -> ptr
  Node *n2 = find_last(n1);           // Call -> ptr
  int *data = get_data_ptr(n1);       // Call -> ptr
  void **generic = get_generic_ptr(); // Call -> ptr
}
