#include<iostream>

struct Node {
  int data;
  Node *next;
  Node(int i) : data(i), next(nullptr) {}
  friend std::ostream &operator<<(std::ostream &os, const Node &n) {
    os << "\nNode\n"
       << "\tdata: " << n.data << "\n\tthis: " << &n << "\n\tnext: " << n.next << "\n";
    return os;
  }
};

Node n = {17};

int main () {

    return 0;

}