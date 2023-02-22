@main
struct MyMain {
    static func main() {
        var _ = wrapper(4)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = x
        i += 3 * 4
        return i
    }
}
