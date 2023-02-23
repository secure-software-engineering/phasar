@main
struct MyMain {
    static func source() -> Int {
        return 0
    }
    static func sink(_ p: Int) {
        print(p)
    }
    static func main() {
        sink(CommandLine.arguments.count)
    }
}
