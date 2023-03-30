var g = 1

@main
struct MyMain {

  static func baz (_ c: Int)-> Int {
    return g + c
  }

  static func bar (_ b: Int)-> Int {
    return baz(b + 1)
  }

  static func foo (_ a: Int)-> Int {
    return bar(a + 1)
  }

    static func main() {
      g += 1
      var i = 0
      i = foo(1)
    }
}
