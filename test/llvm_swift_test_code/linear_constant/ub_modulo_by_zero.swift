@main
struct MyMain {
    static func main() {
        var _ = wrapper(0)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = x
        var j = i % x
        return j
    }
}
