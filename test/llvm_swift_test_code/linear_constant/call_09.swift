@main
struct MyMain {
    static func increment(_ a: Int) -> Int {
        return a + 1
    }

    static func main() {
        var i = increment(42)
        var j = increment(42)
    }
}
