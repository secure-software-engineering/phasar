typedef
#ifdef A
    struct {
  int id;
}
#else
    struct {
  long long id;
}
#endif
id_t;

extern void printId(id_t id);
extern void printInt(
#ifdef A
    int x
#else
    long long x
#endif
);

int main() {
  id_t _myID = {42};

  _myID.id++;

  printId(_myID);
  printInt(_myID.id);
}