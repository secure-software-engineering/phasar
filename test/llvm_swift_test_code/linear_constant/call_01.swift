@main
struct MyMain {
    static func foo(_ a: Int) {
        var b = a
    }

    static func main() {
        var i = 42
        foo(i)
    }
}
