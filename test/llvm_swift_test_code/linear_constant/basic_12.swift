@main
struct MyMain {
    static func main() {
        var i = 42
        i = foo(i)
    }

    static func foo(_ x: Int) -> Int {
        return x + 1
    }

}
