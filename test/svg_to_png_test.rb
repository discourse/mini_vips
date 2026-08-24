# frozen_string_literal: true

require_relative "test_helper"

class SvgToPngTest < Minitest::Test
  def test_converts_svg_fixtures_to_opaque_pngs
    { "image.svg" => [100, 50], "tiny.svg" => [86, 65] }.each do |fixture, dimensions|
      Dir.mktmpdir do |directory|
        output_path = File.join(directory, "output.png")

        run_helper("svg-to-png", fixture_path(fixture), output_path)

        image = assert_png(output_path, width: dimensions[0], height: dimensions[1])
        assert_operator image.pixels.uniq.length, :>, 1
        assert_opaque(image)
      end
    end
  end

  def test_rejects_an_svg_with_an_external_entity
    Dir.mktmpdir do |directory|
      _output, error, status =
        capture_helper(
          "svg-to-png",
          fixture_path("hostile_entity.svg"),
          File.join(directory, "output.png"),
        )

      assert_equal 1, status.exitstatus
      refute_empty error
    end
  end

  def test_rejects_an_svg_without_dimensions
    Dir.mktmpdir do |directory|
      _output, error, status =
        capture_helper(
          "svg-to-png",
          fixture_path("zero_sized.svg"),
          File.join(directory, "output.png"),
        )

      assert_equal 1, status.exitstatus
      refute_empty error
    end
  end
end
