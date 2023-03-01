var g = 10

@main
struct MyMain {
    static func main() {
      var i = g
      i -= 20
      g = i
    }
}
