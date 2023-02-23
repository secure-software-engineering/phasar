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
        var a = source()
        sink(a)
        var s = S(0)  // original C++ code was: S *s = new S(0);
        var b = a
        sink(b)
    }
}
