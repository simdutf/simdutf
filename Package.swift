// swift-tools-version:6.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "simdutf",
    products: [
        .library(
            name: "simdutf",
            targets: ["simdutf"]
        )
    ],
    targets: [
        .target(
            name: "simdutf",
            path: ".",
            sources: [
                "src/simdutf.cpp"
            ],
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("src"),
                .headerSearchPath("include"),
            ]
        )
    ],
    cxxLanguageStandard: .gnucxx20
)
