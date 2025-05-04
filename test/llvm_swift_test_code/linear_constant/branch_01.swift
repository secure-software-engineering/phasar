var cond: Bool = true

@main
struct MyMain {
    static func main() {
        var _ = wrapper(10)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = 10
        if cond {
            i = 2
        }
        return i
    }
}
