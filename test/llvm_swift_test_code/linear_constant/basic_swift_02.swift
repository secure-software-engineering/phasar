@main
struct MyMain {

    static func main() {
        // The code of this method can't be directly placed inside
        // the main function or it would be removed by the compiler
        // due to mandatory optimizations.
        var _ = simpleAdd(x: 1)
    }

    static func simpleAdd(x: Int) -> Int {
        var a = x
        var b = a + 41
        return b
    }
}
