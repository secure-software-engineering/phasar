@main
struct MyMain {
    static func main() {
        var _ = wrapper(3)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = x
        var j = 4 * i + 2
        return j
    }
}
