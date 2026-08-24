# frozen_string_literal: true

require "mini_vips"
require "open3"
require "tmpdir"

run_helper = ->(*arguments) { Open3.capture3(MiniVips.executable, *arguments) }

version_output, version_error, version_status = run_helper.call("version")
abort version_error unless version_status.success?
if Gem::Version.new(version_output.strip) < Gem::Version.new("8.13.0")
  abort "invalid libvips version: #{version_output}"
end

Dir.mktmpdir do |directory|
  font_path = ENV.fetch("MINI_VIPS_TEST_FONT")
  profile_path = ENV.fetch("MINI_VIPS_TEST_PROFILE")
  font_name = font_path.include?("DejaVu") ? "DejaVu Sans 180" : "Arial 180"
  avatar_path = File.join(directory, "avatar.png")
  resized_avatar_path = File.join(directory, "avatar-64.png")
  svg_path = File.join(directory, "input.svg")
  png_path = File.join(directory, "output.png")

  _output, error, status =
    run_helper.call(
      "letter-avatar",
      "--output",
      avatar_path,
      "--markup",
      '<span foreground="#FFFFFF">A</span>',
      "--font",
      font_name,
      "--fontfile",
      font_path,
      "--red",
      "18",
      "--green",
      "52",
      "--blue",
      "86",
    )
  abort error unless status.success? && File.size?(avatar_path)

  _output, error, status =
    run_helper.call(
      "resize-letter-avatar",
      "--input",
      avatar_path,
      "--output",
      resized_avatar_path,
      "--profile",
      profile_path,
      "--size",
      "64",
    )
  abort error unless status.success? && File.size?(resized_avatar_path)

  File.write(
    svg_path,
    '<svg xmlns="http://www.w3.org/2000/svg" width="2" height="2"><rect width="2" height="2" fill="#123456"/></svg>',
  )

  _output, error, status = run_helper.call("topic-og", "--input", svg_path, "--output", png_path)
  abort error unless status.success? && File.size?(png_path)

  color, error, status = run_helper.call("dominant-color", "--input", png_path)
  abort error unless status.success?
  abort "unexpected dominant color: #{color}" unless color.strip == "123456"

  ppm_path = File.join(directory, "blocked.ppm")
  File.write(ppm_path, "P3\n1 1\n255\n18 52 86\n")
  _output, error, status = run_helper.call("dominant-color", "--input", ppm_path)
  abort "blocked loader succeeded" if status.success?
  expected_error =
    error.include?("unsupported input format") || error.include?("operation is blocked")
  abort "unexpected loader error: #{error}" unless expected_error
end
