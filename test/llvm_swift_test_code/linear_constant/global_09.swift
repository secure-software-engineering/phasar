var g1 = 1

@main
struct MyMain {

  static func baz (_ c: Int)-> Int {
    return c + 3
  }

  static func bar (_ b: Int)-> Int {
    g1 += 1
    return b + 1
  }

  static func foo (_ a: Int)-> Int {
    var x = a
    x+=g1
    return x
  }

    static func main() {
      g1 += 1
      var i = 0
      i = foo(10)
      i = bar(3)
      i = baz(39)
    }
}
