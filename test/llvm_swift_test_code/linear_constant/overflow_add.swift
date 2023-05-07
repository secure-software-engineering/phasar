@main
struct MyMain {
    static func main() {
        var _ = wrapper(42)
    }

    static func wrapper(_ x: UInt64) -> UInt64 {
        var i: UInt64 = 18446744073709551614
        var j: UInt64 = i &+ 8
        return j
    }
}
