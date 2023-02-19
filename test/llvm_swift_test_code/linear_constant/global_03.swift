var g = 0

@main
struct MyMain {

  static func foo() {
    g += 1
  }
    static func main() {
      var i = 42
      g += 1
      foo()
    }
}
