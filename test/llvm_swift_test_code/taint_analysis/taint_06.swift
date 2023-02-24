@main
struct MyMain {
    static func source() -> Int {
        return 0
    }
    static func sink(_ p: [String]) {
        print(p)
    }
    static func main() {
        sink(CommandLine.arguments)
    }
}
