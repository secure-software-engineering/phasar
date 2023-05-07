@main
struct MyMain {
    static func bar(_ b: Int) {

    }

    static func foo(_ a: Int) {
        bar(a)
    }

    static func main() {
        var i: Int
        foo(2)
    }
}
