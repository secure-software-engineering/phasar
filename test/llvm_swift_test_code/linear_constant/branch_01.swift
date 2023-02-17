
 var cond: Bool = true

 @main
struct MyMain {
    static func main() {
        var _ = wrapper(10)
    }

    static func wrapper(_ x: Int) -> Int {
        var i = x
        i += 2
        if cond {
            i += 3
        }

        return i
    }
}
