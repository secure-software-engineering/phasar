@main
struct MyMain {
    static func foo(_ a: Int) -> Int {
        return a + 40
    }

    static func main() {
        var i: Int
        i = foo(2)
    }
}
