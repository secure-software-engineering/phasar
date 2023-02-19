@main
struct MyMain {
    static func main() {
        var a = fac(5)
    }

    static func fac(_ i: Int) -> Int {
      if(i == 0) {
        return 1
      }
      return i * fac(i-1)
    }
}
