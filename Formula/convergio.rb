class Convergio < Formula
  desc "Multi-agent AI orchestration CLI for Apple Silicon"
  homepage "https://github.com/Roberdan/convergio-cli"
  version "3.0.10"
  license "MIT"

  on_macos do
    on_arm do
      url "https://github.com/Roberdan/convergio-cli/releases/download/v3.0.10/convergio-3.0.10-darwin-arm64.tar.gz"
      sha256 "20356e286a8edb8d68cbbb60746dfc0323a2193ce497067e7ae162600556eb93"
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
        convergio update       # Check for and install updates

      Documentation: https://github.com/Roberdan/convergio-cli
    EOS
  end

  test do
    assert_match version.to_s, shell_output("#{bin}/convergio --version")
  end
end
