@main
struct MyMain {
    static func main() {
        var _ = wrapper(42)
    }

    static func wrapper(_ x: Int64) -> Int64 {
        var i: Int64 = Int64.min + 1
        var j: Int64 = i &- 8
        var k: Int64 = j &* -1
        return k
    }
}
