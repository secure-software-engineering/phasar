var g = 0

@main
struct MyMain {

  static func foo (_ a: Int)-> Int {
    var x = a
    x += g
    return x
  }

  static func bar (_ b: Int)-> Int {
    g += 1
    return b + 1
  }

    static func main() {
      g += 1
      var i = 0
      i = foo(10)
      i = bar(3)
    }
}
