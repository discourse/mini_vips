# frozen_string_literal: true

require_relative "test_helper"

class VersionTest < Minitest::Test
  def test_prints_the_linked_libvips_version
    output, error, status = capture_helper("version")

    assert status.success?, error
    assert_empty error
    assert_match(/\A8\.\d+\.\d+\n\z/, output)
  end

  def test_rejects_arguments
    assert_command_error(
      "version",
      "unexpected",
      exit_status: 2,
      message: "unexpected argument: unexpected",
    )
  end

  def test_rejects_options
    assert_command_error(
      "version",
      "--unsupported",
      "value",
      exit_status: 2,
      message: "unsupported option --unsupported",
    )
  end
end
