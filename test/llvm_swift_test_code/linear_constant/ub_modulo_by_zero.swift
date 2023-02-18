@main
struct MyMain {
    static func main() {
        var _ = wrapper(42)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = x
        var j = i % 0
        return j
    }
}
