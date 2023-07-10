var g = 0

@_silgen_name("global_ctor") func globalCtor() {
    g = 42
}
@_silgen_name("global_dtor") func globalDtor() {
    g = 666
}

@main
struct MyMain {

  static func foo (_ x: Int)-> Int {
    var a = x
    a = a + 1
    return a
  }

    static func main() {
      var a = g
      var b = foo(a)
    }
}
