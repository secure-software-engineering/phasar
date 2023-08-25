@main
struct MyMain {
    static func source() -> Int {
        return 0
    }
    static func sink(_ p: Int) {
        var b = p
    }
    static func main() {
        var a = source()
        sink(a)
    }
}
