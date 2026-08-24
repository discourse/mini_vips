# frozen_string_literal: true

require_relative "test_helper"

class LetterAvatarTest < Minitest::Test
  def test_matches_debian_letter_avatar_fixtures
    skip unless ENV["MINI_VIPS_TEST_DEBIAN_FIXTURES"] == "true"

    Dir.mktmpdir do |directory|
      ("A".."Z").each do |letter|
        output_path = File.join(directory, "#{letter}.png")

        generate_avatar(letter:, output_path:, background_color: "123456")

        assert FileUtils.compare_file(
                 fixture_path("letter_avatars/debian/#{letter}.png"),
                 output_path,
               ),
               "letter avatar differs for #{letter}"
      end
    end
  end

  def test_generates_the_default_letter_avatar
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "avatar.png")

      generate_avatar(letter: "A", output_path:, background_color: "123456")

      image = assert_png(output_path, width: 360, height: 360)
      assert_letter_avatar(image, background_color: "123456")
    end
  end

  def test_generates_a_custom_size
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "avatar.png")

      generate_avatar(letter: "&", output_path:, background_color: "ABCDEF", size: 90)

      image = assert_png(output_path, width: 90, height: 90)
      assert_letter_avatar(image, background_color: "ABCDEF")
    end
  end

  def test_generates_a_custom_letter_size
    Dir.mktmpdir do |directory|
      small_path = File.join(directory, "small.png")
      large_path = File.join(directory, "large.png")

      generate_avatar(
        letter: "A",
        output_path: small_path,
        background_color: "123456",
        size: 180,
        letter_size: 40,
      )
      generate_avatar(
        letter: "A",
        output_path: large_path,
        background_color: "123456",
        size: 180,
        letter_size: 120,
      )

      background = color("123456")
      small_letter_pixels =
        ChunkyPNG::Image.from_file(small_path).pixels.count { |pixel| pixel != background }
      large_letter_pixels =
        ChunkyPNG::Image.from_file(large_path).pixels.count { |pixel| pixel != background }
      assert_operator large_letter_pixels, :>, small_letter_pixels
    end
  end

  def test_defaults_letter_size_to_seven_ninths_of_avatar_size
    Dir.mktmpdir do |directory|
      default_path = File.join(directory, "default.png")
      explicit_path = File.join(directory, "explicit.png")

      generate_avatar(letter: "A", output_path: default_path, background_color: "123456", size: 99)
      generate_avatar(
        letter: "A",
        output_path: explicit_path,
        background_color: "123456",
        size: 99,
        letter_size: 77,
      )

      assert_equal ChunkyPNG::Image.from_file(explicit_path).pixels,
                   ChunkyPNG::Image.from_file(default_path).pixels
    end
  end

  def test_requires_a_background_color
    assert_command_error(
      "letter-avatar",
      "A",
      "avatar.png",
      exit_status: 2,
      message: "missing required option --background-color",
    )
  end

  def test_rejects_an_invalid_background_color
    assert_command_error(
      "letter-avatar",
      "A",
      "avatar.png",
      "--background-color",
      "invalid",
      exit_status: 2,
      message: "--background-color must be a six-character RGB value",
    )
  end

  def test_rejects_an_invalid_size
    assert_command_error(
      "letter-avatar",
      "A",
      "avatar.png",
      "--background-color",
      "123456",
      "--size",
      "0",
      exit_status: 2,
      message: "--size must be 1..4096",
    )
  end

  def test_rejects_an_invalid_letter_size
    assert_command_error(
      "letter-avatar",
      "A",
      "avatar.png",
      "--background-color",
      "123456",
      "--letter-size",
      "4097",
      exit_status: 2,
      message: "--letter-size must be 1..4096",
    )
  end

  private

  def generate_avatar(letter:, output_path:, background_color:, size: nil, letter_size: nil)
    arguments = ["letter-avatar", letter, output_path, "--background-color", background_color]
    arguments.push("--size", size.to_s) if size
    arguments.push("--letter-size", letter_size.to_s) if letter_size
    run_helper(*arguments)
  end

  def assert_letter_avatar(image, background_color:)
    background = color(background_color)
    assert_equal background, image[0, 0]
    assert image.pixels.any? { |pixel| pixel != background }
    assert_opaque(image)
  end

  def color(hex)
    ChunkyPNG::Color.from_hex("##{hex}")
  end
end
