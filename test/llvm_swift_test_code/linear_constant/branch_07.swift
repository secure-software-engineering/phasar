var cond: Bool = true

@main
struct MyMain {
    static func main() {
        var _ = wrapper(10)
    }

    static func wrapper(_ x: Int) -> Int {
        var j = 10
        var i = j + 20
        if cond {
            i = j + 20
        }
        j = 9
        return x
    }
}
