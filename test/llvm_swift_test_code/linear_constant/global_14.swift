var g = 0

struct X {
  init() {
    g = 1024
  }
}

var v = X()
@main
struct MyMain {

  static func foo (_ x: Int)-> Int {
    var a = x
    a = a + 1
    return a
  }

  static func main() {
    var a = g
    a = foo(a)
  }
}
