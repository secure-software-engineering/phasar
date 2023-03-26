@main
struct MyMain {
    static func main() {
        var _ = wrapper(42)
    }

    static func wrapper(_ x: UInt64) -> UInt64 {
        var i: UInt64 = 18_446_744_073_709_551_614
        var j: UInt64 = i &+ 8
        return j
    }
}
