@main
struct MyMain {
    static func foo() -> Int {
        return 42
    }

    static func main() {
        var i = foo()
    }
}
