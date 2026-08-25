# frozen_string_literal: true

require "minitest/autorun"
require "chunky_png"
require "fileutils"
require "open3"
require "rbconfig"
require "tmpdir"
require "mini_vips"

module MiniVipsTestHelpers
  FIXTURES_PATH = File.expand_path("fixtures/images", __dir__)
  private_constant :FIXTURES_PATH

  private

  def fixture_path(name)
    File.join(FIXTURES_PATH, name)
  end

  def capture_helper(*arguments)
    Open3.capture3(MiniVips.executable, *arguments)
  end

  def run_helper(*arguments)
    output, error, status = capture_helper(*arguments)
    assert status.success?, error
    assert_empty error
    output
  end

  def assert_command_error(*arguments, exit_status:, message:)
    _output, error, status = capture_helper(*arguments)

    assert_equal exit_status, status.exitstatus
    assert_includes error, message
  end

  def assert_png(path, width:, height:)
    image = ChunkyPNG::Image.from_file(path)

    assert_equal [width, height], [image.width, image.height]
    image
  end

  def assert_opaque(image)
    assert image.pixels.all? { |pixel| ChunkyPNG::Color.a(pixel) == 255 }
  end
end

class Minitest::Test
  include MiniVipsTestHelpers
end
