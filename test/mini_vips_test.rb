# frozen_string_literal: true

require_relative "test_helper"

class MiniVipsTest < Minitest::Test
  def test_exposes_the_cli_asset_paths
    assert_equal ["NotoSans-Regular.ttf"], MiniVips.asset_paths.map { |path| File.basename(path) }
    assert MiniVips.asset_paths.all? { |path| File.file?(path) }
  end
end
