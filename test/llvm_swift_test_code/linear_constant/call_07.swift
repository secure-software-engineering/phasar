@main
struct MyMain {
    static func increment(_ a: Int) -> Int {
        return a + 1
    }

    static func main() {
        var i = 42
        var j = increment(i)
        var k = increment(j)
    }
}
