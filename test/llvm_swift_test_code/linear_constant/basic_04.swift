@main
struct MyMain {
    static func main() {
        var _ = addWrapper(14)
    }

    static func addWrapper(_ x: Int) -> Int {
        var i = x
        var j = 20
        var k = i + j
        return k
    }
}
