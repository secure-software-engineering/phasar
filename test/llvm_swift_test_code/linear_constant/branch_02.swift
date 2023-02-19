var cond: Bool = true

@main
struct MyMain {
    static func main() {
        var _ = wrapper(10)
    }

    static func wrapper(_ x: Int) -> Int {
        var i: Int
        if cond {
            i = 10
        }
        return x
    }
}
