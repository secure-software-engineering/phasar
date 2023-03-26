@_silgen_name("source")
func source() -> Int

@_silgen_name("sink")
func sink(_ p: [String])

@main
struct MyMain {
    static func main() {
        // We changed this from the original C++ test
        // which checked argc.
        sink(CommandLine.arguments)
    }
}
