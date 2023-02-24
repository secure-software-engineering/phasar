@_silgen_name("source")
func source() -> Int

@_silgen_name("sink")
func sink(_ p: Int)

struct S {
    var data: Int
    init(_ data: Int) {
        self.data = data
    }
    func test() throws {}
}
@main
struct MyMain {
    static func main() {
        var data = source()
        do {
            var s = S(0)
            try s.test()
        } catch {
            sink(data)
        }
    }
}
