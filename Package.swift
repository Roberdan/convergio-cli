// swift-tools-version:5.9
// Package.swift - Swift integration for Convergio
// - MLX local LLM inference
// - Apple Foundation Models (macOS 26+)

import PackageDescription

let package = Package(
    name: "ConvergioMLX",
    platforms: [
        .macOS(.v14)  // MLX requires macOS 14+, AFM requires macOS 26+
    ],
    products: [
        .library(
            name: "ConvergioMLX",
            type: .static,
            targets: ["ConvergioMLX"]
        ),
        .library(
            name: "ConvergioAFM",
            type: .static,
            targets: ["ConvergioAFM"]
        )
    ],
    dependencies: [
        // MLX Swift LM - contains MLXLLM, MLXLMCommon (pinned to stable release)
        .package(url: "https://github.com/ml-explore/mlx-swift-lm.git", exact: "2.29.2"),
    ],
    targets: [
        .target(
            name: "ConvergioMLX",
            dependencies: [
                .product(name: "MLXLLM", package: "mlx-swift-lm"),
                .product(name: "MLXLMCommon", package: "mlx-swift-lm"),
            ],
            path: "Sources/ConvergioMLX",
            swiftSettings: [
                .unsafeFlags(["-O", "-whole-module-optimization"])
            ]
        ),
        // Apple Foundation Models bridge (macOS 26+ only)
        .target(
            name: "ConvergioAFM",
            dependencies: [],
            path: "Sources/ConvergioAFM",
            swiftSettings: [
                .unsafeFlags(["-O", "-whole-module-optimization", "-target", "arm64-apple-macos26.0"])
            ]
        )
    ]
)
