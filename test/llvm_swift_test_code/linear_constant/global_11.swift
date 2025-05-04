var g1 = 42
var g2 = 9001

@main
struct MyMain {

  static func foo (_ x: Int)-> Int {
    var a = x
    a = a + 1
    return a
  }


    static func main() {
      var a = 13
      a = foo(a)
    }
}
