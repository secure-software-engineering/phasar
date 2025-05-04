var g = 0

@main
struct MyMain {

  static func foo (_ a: Int)-> Int {
    return a + 1
  }

    static func main() {
      g += 1
      var i = foo(g)
    }
}
