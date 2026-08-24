# frozen_string_literal: true

require_relative "test_helper"

class ResizePngTest < Minitest::Test
  def test_resizes_png_fixtures_to_a_square
    %w[logo.png crop_position.png 2000x2000.png].each do |fixture|
      Dir.mktmpdir do |directory|
        output_path = File.join(directory, fixture)

        run_helper("resize-png", fixture_path(fixture), output_path, "--size", "64")

        image = assert_png(output_path, width: 64, height: 64)
        if fixture == "crop_position.png"
          assert_equal ChunkyPNG::Color.rgb(255, 0, 0), image[32, 0]
          assert_equal ChunkyPNG::Color.rgb(0, 0, 255), image[32, 63]
        elsif fixture == "2000x2000.png"
          assert_equal [ChunkyPNG::Color::WHITE], image.pixels.uniq
        else
          assert_operator image.pixels.uniq.length, :>, 1
        end
      end
    end
  end

  def test_rejects_a_non_png_fixture
    Dir.mktmpdir do |directory|
      _output, error, status =
        capture_helper(
          "resize-png",
          fixture_path("logo.jpg"),
          File.join(directory, "logo.png"),
          "--size",
          "64",
        )

      assert_equal 1, status.exitstatus
      refute_empty error
    end
  end

  def test_requires_a_size
    assert_command_error(
      "resize-png",
      fixture_path("logo.png"),
      "output.png",
      exit_status: 2,
      message: "--size must be 1..4096",
    )
  end

  def test_rejects_an_invalid_size
    assert_command_error(
      "resize-png",
      fixture_path("logo.png"),
      "output.png",
      "--size",
      "invalid",
      exit_status: 2,
      message: "invalid integer for --size",
    )
  end
end
