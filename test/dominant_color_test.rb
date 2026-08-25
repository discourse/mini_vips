# frozen_string_literal: true

require_relative "test_helper"

class DominantColorTest < Minitest::Test
  def test_prints_a_color_for_supported_image_fixtures
    %w[
      logo.png
      logo.jpg
      tiny_animated.gif
      animated.webp
      should_be_jpeg.heic
      logo.jxl
    ].each do |fixture|
      color = run_helper("dominant-color", fixture_path(fixture))

      assert_match(/\A[0-9A-F]{6}\n\z/, color, fixture)
    end
  end

  def test_prints_the_color_of_a_uniform_fixture
    assert_equal "FFFFFF\n", run_helper("dominant-color", fixture_path("2000x2000.png"))
  end

  def test_treats_transparency_as_black
    assert_equal "7F0000\n", run_helper("dominant-color", fixture_path("transparent.png"))
    assert_equal "400040\n", run_helper("dominant-color", fixture_path("mixed_transparency.png"))
  end

  def test_rejects_an_unsupported_fixture
    assert_command_error(
      "dominant-color",
      fixture_path("fake.jpg"),
      exit_status: 1,
      message: "unsupported input format",
    )
  end

  def test_rejects_a_broken_fixture
    _output, error, status = capture_helper("dominant-color", fixture_path("broken.png"))

    assert_equal 1, status.exitstatus
    refute_empty error
  end

  def test_rejects_a_tiff_fixture
    assert_command_error(
      "dominant-color",
      fixture_path("tiff_as.bin"),
      exit_status: 1,
      message: "unsupported input format",
    )
  end

  def test_rejects_filename_options
    assert_command_error(
      "dominant-color",
      "#{fixture_path("tiny_animated.gif")}[n=-1]",
      exit_status: 2,
      message: "input filename options are not supported",
    )
  end
end
