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
        var data: Int = 0  // Swift does not support uninitilized variables as given in the C++ test case
        do {
            var s = S(0)
            do {
                var s = S(0)
            } catch {
                data = source()
            }
        } catch {

        }
        sink(data)
    }
}
