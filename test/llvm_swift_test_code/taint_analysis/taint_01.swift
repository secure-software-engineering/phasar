@_silgen_name("source")
func source() -> Int

@_silgen_name("sink")
func sink(_ p: Int)

@main
struct MyMain {
    static func main() {
        var a = source()
        sink(a)
    }
}
