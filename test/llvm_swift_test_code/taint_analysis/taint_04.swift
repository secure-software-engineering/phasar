@main
struct MyMain {
    static func source() -> Int {
        return 0
    }
    static func sink(_ p: Int) {
        print(p)
    }
    static func main() {
        var a = source()
        sink(a)
        var b = a
        sink(b)
    }
}
