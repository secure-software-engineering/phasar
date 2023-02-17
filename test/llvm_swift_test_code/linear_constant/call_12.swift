@main
struct MyMain {
    static func foo(_ a: Int, _ b: Int) -> Int {
        var x = a
        var y = b
        x += y
        y += a
        return x + y
    }

    static func main() {
        var k: Int
        k = foo(1, 2)
    }
}
