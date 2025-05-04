@main
struct MyMain {
    static func main() {
        var a = foo(5)
    }

    static func foo(_ i: Int) -> Int {
      if(i == 0) {
        return 1
      }
      return foo(i-1)
    }
}
