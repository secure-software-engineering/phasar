// merge() strands the absorbed pts diff in Rep's PendingPts.
// handlePhi interns the loop-carried GEP first, so its still-empty node wins
// the union-find join over the load it is later merged with -- and
// addAssignEdge re-marks Rep's pts only when that pts is non-empty.
struct Node {
  struct Node *Next;
};

struct Node Obj;

void sink(struct Node *P) { (void)P; }

struct Node *walk(struct Node **Slot, struct Node *Init, int N) {
  struct Node *P = Init;
  for (int I = 0; I < N; ++I) {
    struct Node *Base = *Slot;
    sink(P); // forces a propagate(), so Base has pointees at the GEP below
    P = Base + 1;
  }
  return P;
}

int main(int argc, char **argv) {
  struct Node Local;
  struct Node *Slot = &Local;
  struct Node *R = walk(&Slot, &Obj, argc);
  return R != 0;
}
