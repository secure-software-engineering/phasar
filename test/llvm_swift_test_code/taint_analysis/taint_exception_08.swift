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
        var data = 0  // Swift does not support uninitilized variables as given in the C++ test case
        do {
            var s = S(0)
            try s.test()
            do {
                data = source()
                try s.test()
            } catch {}
        } catch {

        }
        sink(data)
    }
}
