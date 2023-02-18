var rand = true
@main
struct MyMain {
  static func foo(_ x: Int) -> Int {
    var a = x
    while (rand) {
      a += 1
    }
    return a
  }

    static func main() {
      var a = 3
      var b = foo(a)
    }
}
