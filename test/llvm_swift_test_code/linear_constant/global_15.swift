var g1 = 0
var g2 = 99

struct X {
  init() {
    g1 = 1024
  }
}

class Y {
  init() {
    g2 = g2 + 1
  }
  deinit {
    g1 = g2 + 13
  }
}

var v = X()
var w = Y()
@main
struct MyMain {

  static func foo (_ x: Int)-> Int {
    var a = x
    a = a + 1
    return a
  }

  static func main() {
    var a = g1
    var b = g2
    a = foo(a)
    a = a + 30
  }
}
