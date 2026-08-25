# frozen_string_literal: true

require_relative "mini_vips/version"

module MiniVips
  EXECUTABLE_PATH = File.expand_path("../libexec/mini_vips", __dir__)
  private_constant :EXECUTABLE_PATH

  ASSET_PATHS = [File.expand_path("mini_vips/fonts/NotoSans-Regular.ttf", __dir__)].freeze
  private_constant :ASSET_PATHS

  def self.executable
    return EXECUTABLE_PATH if File.file?(EXECUTABLE_PATH) && File.executable?(EXECUTABLE_PATH)

    raise "mini_vips executable is unavailable at #{EXECUTABLE_PATH}"
  end

  def self.asset_paths
    ASSET_PATHS
  end
end
