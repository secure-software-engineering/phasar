@main
struct MyMain {
    static func main() {
        var _ = wrapper(4)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = x
        var j = 3
        i += j * 4
        return i
    }
}
