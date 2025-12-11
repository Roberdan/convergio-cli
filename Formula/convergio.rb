class Convergio < Formula
  desc "Multi-agent AI orchestration CLI for Apple Silicon"
  homepage "https://github.com/Roberdan/convergio-cli"
  version "1.0.0"
  license "MIT"

  on_macos do
    on_arm do
      url "https://github.com/Roberdan/convergio-cli/releases/download/v1.0.0/convergio-1.0.0-arm64-apple-darwin.tar.gz"
      sha256 "PLACEHOLDER_WILL_BE_UPDATED_BY_CI"
    end
  end

  depends_on :macos
  depends_on arch: :arm64

  def install
    bin.install "convergio"
  end

  def caveats
    <<~EOS
      Convergio has been installed!

      To get started, run:
        convergio setup

      This will configure your Anthropic API key securely in macOS Keychain.

      Quick start:
        convergio              # Start interactive session with Ali
        convergio --help       # Show all options
        convergio update check # Check for updates

      Documentation: https://github.com/Roberdan/convergio-cli
    EOS
  end

  test do
    assert_match version.to_s, shell_output("#{bin}/convergio --version")
  end
end
