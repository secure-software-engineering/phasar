var cond = true
@main
struct MyMain {
    static func main() {
        var j = decrement(-42)
    }

    static func decrement(_ i: Int) -> Int {
      if(cond) {
        return decrement(i - 1)
      }
      return -1
    }
}
