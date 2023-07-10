@main
struct MyMain {
    static func foo(_ a: Int, _ b: Int) -> Int {
        return a + b
    }

    static func main() {
        var i = 10
        var j = 1
        var k: Int
        k = foo(i, j)
    }
}
