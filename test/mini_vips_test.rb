# frozen_string_literal: true

require "minitest/autorun"
require "fileutils"
require "open3"
require "tmpdir"
require "mini_vips"

class MiniVipsTest < Minitest::Test
  FONT_PATHS = %w[
    /System/Library/Fonts/Helvetica.ttc
    /System/Library/Fonts/Supplemental/Arial.ttf
    /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  ].freeze
  private_constant :FONT_PATHS

  def test_gem_version
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

  def test_version_command
    assert_match(/\A8\.\d+\.\d+\n\z/, run_helper("version"))
  end

  def test_invalid_commands_return_usage_error
    cases = [
      [%w[dominant-color], "missing required argument in"],
      [%w[version unexpected], "unexpected argument: unexpected"],
      [%w[version --option one --option two], "duplicate option --option"],
      [
        ["letter-avatar", "A", "avatar.png", "--font-family", "Noto Sans"],
        "missing required option --background-color",
      ],
      [
        [
          "letter-avatar",
          "A",
          "avatar.png",
          "--font-family",
          "Noto Sans",
          "--background-color",
          "invalid",
        ],
        "--background-color must be a six-character RGB value",
      ],
      [%w[resize-png input.png output.png --size invalid], "invalid integer for --size"],
      [%w[resize-png input.png output.png --size 0], "--size must be 1..4096"],
      [%w[version --unsupported value], "unsupported option --unsupported"],
    ]

    cases.each do |arguments, expected_error|
      _output, error, status = Open3.capture3(MiniVips.executable, *arguments)

      assert_equal 2, status.exitstatus, arguments.join(" ")
      assert_includes error, expected_error, arguments.join(" ")
    end
  end

  def test_letter_avatar_command
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "avatar.png")

      generate_avatar(output_path)

      assert File.size?(output_path)
    end
  end

  def test_letter_avatar_command_with_installed_font
    Dir.mktmpdir do |directory|
      output_path = File.join(directory, "avatar.png")

      generate_avatar(output_path, include_font_file: false)

      assert File.size?(output_path)
    end
  end

  def test_resize_png_command
    Dir.mktmpdir do |directory|
      input_path = File.join(directory, "avatar.png")
      output_path = File.join(directory, "avatar-64.png")
      generate_avatar(input_path)

      run_helper("resize-png", input_path, output_path, "--size", "64")

      assert File.size?(output_path)
    end
  end

  def test_dominant_color_command
    Dir.mktmpdir do |directory|
      input_path = File.join(directory, "input.svg")
      File.write(
        input_path,
        '<svg xmlns="http://www.w3.org/2000/svg" width="2" height="2"><rect width="2" height="2" fill="#123456"/></svg>',
      )

      output_path = File.join(directory, "input.png")
      run_helper("svg-to-png", input_path, output_path)

      assert_equal "123456\n", run_helper("dominant-color", output_path)
    end
  end

  def test_svg_to_png_command
    Dir.mktmpdir do |directory|
      input_path = File.join(directory, "input.svg")
      output_path = File.join(directory, "output.png")
      File.write(
        input_path,
        '<svg xmlns="http://www.w3.org/2000/svg" width="2" height="2"><rect width="2" height="2" fill="#123456"/></svg>',
      )

      run_helper("svg-to-png", input_path, output_path)

      assert File.size?(output_path)
    end
  end

  def test_dominant_color_blocks_untrusted_loaders
    Dir.mktmpdir do |directory|
      input_path = File.join(directory, "input.ppm")
      File.write(input_path, "P3\n1 1\n255\n18 52 86\n")

      _output, error, status = Open3.capture3(MiniVips.executable, "dominant-color", input_path)

      refute status.success?
      assert_match(/unsupported input format|operation is blocked/, error)
    end
  end

  private

  def generate_avatar(output_path, include_font_file: true)
    arguments = [
      "letter-avatar",
      "A",
      output_path,
      "--font-family",
      font_family,
      "--background-color",
      "123456",
    ]
    arguments.push("--font-file", font_path) if include_font_file
    run_helper(*arguments)
  end

  def run_helper(*arguments)
    output, error, status = Open3.capture3(MiniVips.executable, *arguments)
    assert status.success?, error
    output
  end

  def font_path
    @font_path ||= FONT_PATHS.find { |path| File.file?(path) } || raise("No test font found")
  end

  def font_family
    return "DejaVu Sans" if File.basename(font_path).start_with?("DejaVu")
    return "Helvetica" if File.basename(font_path).start_with?("Helvetica")

    "Arial"
  end
end
