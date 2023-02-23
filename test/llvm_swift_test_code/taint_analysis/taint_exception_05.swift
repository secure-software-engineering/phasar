@_silgen_name("source")
func source() -> Int

@_silgen_name("sink")
func sink(_ p: Int)

struct S {
    var data: Int
    init(_ data: Int) {
        self.data = data
    }
}
@main
struct MyMain {
    static func main() {
        var data = 0
        do {
            var s = S(0)

        } catch {
            data = source()
        }
        sink(data)
    }
}
