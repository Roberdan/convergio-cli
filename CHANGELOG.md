# Changelog

## [0.1.9](https://github.com/Roberdan/convergio-cli/compare/v0.1.8...v0.1.9) (2026-04-18)


### Bug Fixes

* **cli:** long-running HTTP client for doctor full + stress ([4580ff9](https://github.com/Roberdan/convergio-cli/commit/4580ff9780486fc0c1ad76d778cc3b72850020b3))
* **cli:** use long-running HTTP client for doctor full + stress endpoints ([beaf3d9](https://github.com/Roberdan/convergio-cli/commit/beaf3d931db116727b28665a9223344ed2fb033d))
* **deps:** bump rustls-webpki 0.103.11 -&gt; 0.103.12 (RUSTSEC-2026-0099) ([6f20eb7](https://github.com/Roberdan/convergio-cli/commit/6f20eb764ff227c2c6f8c362f6bfacfaffd90fa0))

## [0.1.8](https://github.com/Roberdan/convergio-cli/compare/v0.1.7...v0.1.8) (2026-04-15)


### Bug Fixes

* implement all cvg agent subcommands (create/start/complete/enable/disable/sync/history) ([ce7a742](https://github.com/Roberdan/convergio-cli/commit/ce7a7424f2269d2b9c5ec9f0b098afd6c6789b30))

## [0.1.7](https://github.com/Roberdan/convergio-cli/compare/v0.1.6...v0.1.7) (2026-04-14)


### Bug Fixes

* read platform version from /api/health instead of /api/deploy/status ([680496d](https://github.com/Roberdan/convergio-cli/commit/680496dfa283b1bca455b6d5dc56f1a337111e93))

## [0.1.6](https://github.com/Roberdan/convergio-cli/compare/v0.1.5...v0.1.6) (2026-04-13)


### Bug Fixes

* pass CARGO_REGISTRY_TOKEN to release workflow ([56f5006](https://github.com/Roberdan/convergio-cli/commit/56f50065d4f688fa5fc7d3ac5709ddb3702d4331))

## [0.1.5](https://github.com/Roberdan/convergio-cli/compare/v0.1.4...v0.1.5) (2026-04-13)


### Bug Fixes

* add crates.io publishing metadata (description, repository) ([9a8b552](https://github.com/Roberdan/convergio-cli/commit/9a8b55206ee53f71ef22ee885a43e9b8f2b87f67))

## [0.1.4](https://github.com/Roberdan/convergio-cli/compare/v0.1.3...v0.1.4) (2026-04-13)


### Features

* add cvg chain command — ecosystem status + cascade bump ([1a6b1b5](https://github.com/Roberdan/convergio-cli/commit/1a6b1b5ae34e990bea151e7e44dae6c24d347829))
* add cvg chain command — ecosystem status + cascade bump ([8257d07](https://github.com/Roberdan/convergio-cli/commit/8257d072025d2454521e1736ff9acaab2ccfbf2c))


### Bug Fixes

* propagate chain dispatch errors to caller ([81978f0](https://github.com/Roberdan/convergio-cli/commit/81978f0f99dcbeb2d52c8fc9f2dfa9d1dfbf7155))

## [0.1.3](https://github.com/Roberdan/convergio-cli/compare/v0.1.2...v0.1.3) (2026-04-13)


### Features

* adapt convergio-cli for standalone repo ([4fe423f](https://github.com/Roberdan/convergio-cli/commit/4fe423f895d3b52e44f2112c3208f739f3282742))


### Bug Fixes

* Add KeyInit import for Hmac API change in v0.1.9 ([e770000](https://github.com/Roberdan/convergio-cli/commit/e770000536c207381fb6afc344f9506cae92caf3))
* comprehensive security audit — hardened HTTP, path traversal, secret perms ([#93](https://github.com/Roberdan/convergio-cli/issues/93)) ([1b40f37](https://github.com/Roberdan/convergio-cli/commit/1b40f3704d7a1aeaa18e78847c87346e85c935fa))
* **release:** use vX.Y.Z tag format (remove component) ([569fc13](https://github.com/Roberdan/convergio-cli/commit/569fc13c349dcb885d696345fa09663320e23955))


### Documentation

* add .env.example with required environment variables ([#6](https://github.com/Roberdan/convergio-cli/issues/6)) ([2c4fd52](https://github.com/Roberdan/convergio-cli/commit/2c4fd52e79e3279f894a382170bf51985e7104ab))

## [0.1.2](https://github.com/Roberdan/convergio-cli/compare/convergio-cli-v0.1.1...convergio-cli-v0.1.2) (2026-04-12)


### Bug Fixes

* Add KeyInit import for Hmac API change in v0.1.9 ([e770000](https://github.com/Roberdan/convergio-cli/commit/e770000536c207381fb6afc344f9506cae92caf3))

## [0.1.1](https://github.com/Roberdan/convergio-cli/compare/convergio-cli-v0.1.0...convergio-cli-v0.1.1) (2026-04-12)


### Features

* adapt convergio-cli for standalone repo ([4fe423f](https://github.com/Roberdan/convergio-cli/commit/4fe423f895d3b52e44f2112c3208f739f3282742))


### Bug Fixes

* comprehensive security audit — hardened HTTP, path traversal, secret perms ([#93](https://github.com/Roberdan/convergio-cli/issues/93)) ([1b40f37](https://github.com/Roberdan/convergio-cli/commit/1b40f3704d7a1aeaa18e78847c87346e85c935fa))


### Documentation

* add .env.example with required environment variables ([#6](https://github.com/Roberdan/convergio-cli/issues/6)) ([2c4fd52](https://github.com/Roberdan/convergio-cli/commit/2c4fd52e79e3279f894a382170bf51985e7104ab))

## 0.1.0 (Initial Release)

### Features

- Initial extraction from convergio monorepo
