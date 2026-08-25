# frozen_string_literal: true

require_relative "test_helper"

class ResizeTest < Minitest::Test
  def test_covers_from_the_center_or_top
    Dir.mktmpdir do |directory|
      center_path = File.join(directory, "center.png")
      top_path = File.join(directory, "top.png")

      resize("crop_position.png", center_path, "--width", "3", "--height", "2", "--fit", "cover")
      resize(
        "crop_position.png",
        top_path,
        "--width",
        "3",
        "--height",
        "2",
        "--fit",
        "cover",
        "--position",
        "top",
      )

      center = assert_png(center_path, width: 3, height: 2)
      top = assert_png(top_path, width: 3, height: 2)
      assert_operator ChunkyPNG::Color.r(center[1, 1]), :<, ChunkyPNG::Color.b(center[1, 1])
      assert top.pixels.all? { |pixel| ChunkyPNG::Color.r(pixel) > ChunkyPNG::Color.b(pixel) }
    end
  end

  def test_contains_without_enlargement
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.png")

      resize("logo.png", output_path, "--width", "500", "--height", "500", "--without-enlargement")

      assert_png(output_path, width: 244, height: 66)
    end
  end

  def test_scales_proportionally
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.png")

      resize("7x11.svg", output_path, "--scale", "0.333")

      assert_png(output_path, width: 2, height: 4)
    end
  end

  def test_selects_the_output_encoder_from_the_extension
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.webp")

      resize("logo.jpg", output_path, "--scale", "0.5", "--quality", "80")

      assert_equal "RIFF", File.binread(output_path, 4)
    end
  end

  def test_writes_jpeg_xl_output
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.jxl")

      resize("logo.png", output_path, "--scale", "0.5")

      assert_equal "\x00\x00\x00\x0CJXL \r\n\x87\n".b, File.binread(output_path, 12)
    end
  end

  def test_limits_pixel_area_without_enlargement
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.png")

      resize("2000x2000.png", output_path, "--max-pixels", "10000")

      assert_png(output_path, width: 100, height: 100)
    end
  end

  def test_preserves_transparency
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.png")

      resize("transparent.png", output_path, "--scale", "10")

      image = assert_png(output_path, width: 10, height: 10)
      assert image.pixels.any? { |pixel| ChunkyPNG::Color.a(pixel) < 255 }
    end
  end

  def test_limits_palette_colors_and_strips_metadata
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "output.png")

      resize(
        "logo.png",
        output_path,
        "--width",
        "50",
        "--height",
        "50",
        "--colors",
        "12",
        "--strip-metadata",
      )

      image = ChunkyPNG::Image.from_file(output_path)
      assert_operator image.pixels.uniq.length, :<=, 12
      refute_includes File.binread(output_path), "eXIf"
    end
  end

  def test_rejects_an_unsupported_input
    assert_command_error(
      "resize",
      fixture_path("tiff_as.bin"),
      "output.png",
      "--scale",
      "0.5",
      exit_status: 1,
      message: "unsupported input format",
    )
  end

  def test_rejects_filename_options
    assert_command_error(
      "resize",
      "#{fixture_path("tiny_animated.gif")}[n=-1]",
      "output.png",
      "--scale",
      "1",
      exit_status: 2,
      message: "input filename options are not supported",
    )
  end

  def test_allows_brackets_in_directories
    Dir.mktmpdir do |directory|
      bracketed_directory = File.join(directory, "images[1]")
      FileUtils.mkdir_p(bracketed_directory)
      input_path = File.join(bracketed_directory, "input.png")
      output_path = File.join(directory, "output.png")
      FileUtils.cp(fixture_path("logo.png"), input_path)

      run_helper("resize", input_path, output_path, "--scale", "0.5")

      assert_png(output_path, width: 122, height: 33)
    end
  end

  def test_rejects_an_unsupported_output
    assert_command_error(
      "resize",
      fixture_path("logo.png"),
      "output.tiff",
      "--scale",
      "0.5",
      exit_status: 2,
      message: "unsupported output format",
    )
  end

  def test_requires_one_resize_selector
    assert_command_error(
      "resize",
      fixture_path("logo.png"),
      "output.png",
      exit_status: 2,
      message: "specify exactly one",
    )
  end

  private

  def resize(fixture, output_path, *options)
    run_helper("resize", fixture_path(fixture), output_path, *options)
  end
end
