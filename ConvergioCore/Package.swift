// swift-tools-version:5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "ConvergioCore",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .library(
            name: "ConvergioCore",
            targets: ["ConvergioCore"]
        ),
    ],
    dependencies: [],
    targets: [
        // C bridging target - exposes the C library to Swift
        .target(
            name: "CConvergio",
            dependencies: [],
            path: "Sources/CConvergio",
            publicHeadersPath: ".",
            cSettings: [
                .headerSearchPath("../include"),
                .define("CONVERGIO_NATIVE_APP", to: "1")
            ],
            linkerSettings: [
                // Link against the static library (built by CMake)
                .unsafeFlags(["-L../build/lib"]),
                .linkedLibrary("convergio"),
                // System dependencies
                .linkedFramework("Accelerate"),
                .linkedFramework("Metal"),
                .linkedFramework("Foundation"),
                .linkedFramework("AppKit"),
                .linkedLibrary("curl"),
                .linkedLibrary("sqlite3"),
                .linkedLibrary("z")
            ]
        ),
        // Swift wrapper target
        .target(
            name: "ConvergioCore",
            dependencies: ["CConvergio"],
            path: "Sources/ConvergioCore",
            swiftSettings: [
                .enableExperimentalFeature("StrictConcurrency")
            ]
        ),
        // Tests
        .testTarget(
            name: "ConvergioCoreTests",
            dependencies: ["ConvergioCore"],
            path: "Tests/ConvergioCoreTests"
        ),
    ]
)
