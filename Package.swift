// swift-tools-version:5.9
// Package.swift - MLX Swift integration for Convergio local LLM inference

import PackageDescription

let package = Package(
    name: "ConvergioMLX",
    platforms: [
        .macOS(.v14)  // MLX requires macOS 14+
    ],
    products: [
        .library(
            name: "ConvergioMLX",
            type: .static,
            targets: ["ConvergioMLX"]
        )
    ],
    dependencies: [
        // MLX Swift LM - contains MLXLLM, MLXLMCommon (use main branch for latest)
        .package(url: "https://github.com/ml-explore/mlx-swift-lm.git", branch: "main"),
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
        )
    ]
)
