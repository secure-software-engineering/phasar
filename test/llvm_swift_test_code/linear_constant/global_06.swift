var g = 0

@main
struct MyMain {

  static func foo ()-> Int {
    return g + 1
  }

    static func main() {
      g += 1
      var i = foo()
    }
}
