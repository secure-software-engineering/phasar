@main
struct MyMain {
    static func bar(_ b: Int)-> Int {
        return b
    }

    static func foo(_ a: Int)-> Int {
        return bar(a)
    }

    static func main() {
        var i: Int
        i = foo(2)
    }
}
