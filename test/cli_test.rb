# frozen_string_literal: true

require_relative "test_helper"

class CliTest < Minitest::Test
  def test_requires_a_command
    assert_command_error(exit_status: 2, message: "usage:")
  end

  def test_rejects_an_unknown_command
    assert_command_error("unknown", exit_status: 2, message: "unsupported helper command")
  end

  def test_rejects_duplicate_options
    assert_command_error(
      "version",
      "--option",
      "one",
      "--option",
      "two",
      exit_status: 2,
      message: "duplicate option --option",
    )
  end
end
