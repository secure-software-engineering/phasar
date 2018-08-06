export class RingListNode<T>{
    private data: T;
    private next: RingListNode<T>;
    private previous: RingListNode<T>;
    constructor(data: T, previous?: RingListNode<T>) {
        this.data = data;
        this.previous = previous;
        this.next = undefined;
    }
    public getData() {
        return this.data;
    }
    public setNext(next: RingListNode<T>) {
        this.next = next
    }
    public setPrevious(previous: RingListNode<T>) {
        this.previous = previous;
    }
    public getNext() {
        return this.next;
    }
    public getPrevious() {
        return this.previous;
    }
}

export class RingList<T> {
    private size: number;

    private head: RingListNode<T>;
    private tail: RingListNode<T>;

    private cursor: RingListNode<T>;

    constructor() {
        this.size = 0;
        this.next = this.next.bind(this);
        this.previous = this.previous.bind(this);
    }

    public next(): RingListNode<T> {

        this.cursor = this.cursor.getNext();
        return this.cursor;
    }

    public previous(): RingListNode<T> {
        this.cursor = this.cursor.getPrevious();
        return this.cursor;

    }

    public getTail(): RingListNode<T> {
        return this.tail;
    }
    public getHead(): RingListNode<T> {
        return this.head;
    }

    public add(data: T) {
        let n: RingListNode<T>;
        if (this.size == 0) {
            n = new RingListNode<T>(data);
            this.head = n;
            this.head.setNext(n);
            this.cursor = n;
            this.tail = n;
        }
        else {
            n = new RingListNode<T>(data, this.tail);
        }

        this.tail.setNext(n);
        this.tail = n;
        this.tail.setNext(this.head);
        this.head.setPrevious(this.tail);

        this.size = this.size + 1;
    }
}