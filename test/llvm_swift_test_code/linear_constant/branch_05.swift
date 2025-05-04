var cond: Bool = true

@main
struct MyMain {
    static func main() {
        var _ = wrapper(10)
    }

    static func wrapper(_ x: Int) -> Int {
        var j = 10
        var i = 42
        if cond {
            i = j + 32
        }
        return x
    }
}
