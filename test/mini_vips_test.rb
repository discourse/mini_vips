# frozen_string_literal: true

require "minitest/autorun"
require "fileutils"
require "open3"
require "tmpdir"
require_relative "../lib/mini_vips"

class MiniVipsTest < Minitest::Test
  def test_version
    assert_equal "0.1.0", MiniVips::VERSION
  end

  def test_executable
    executable = MiniVips.executable

    assert File.file?(executable)
    assert File.executable?(executable)
    assert_equal "mini_vips", File.basename(executable)
  end

  def test_executable_fails_clearly_when_unavailable
    Dir.mktmpdir do |directory|
      FileUtils.cp_r("lib", directory)

      _output, error, status =
        Open3.capture3(
          RbConfig.ruby,
          "--disable-gems",
          "-I#{File.join(directory, "lib")}",
          "-rmini_vips",
          "-e",
          "MiniVips.executable",
        )

      refute status.success?
      assert_includes error, "mini_vips executable is unavailable"
    end
  end
end
