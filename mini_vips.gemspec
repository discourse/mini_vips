# frozen_string_literal: true

require "rbconfig"
require_relative "lib/mini_vips/version"

host_cpu = RbConfig::CONFIG.fetch("host_cpu")
host_os = RbConfig::CONFIG.fetch("host_os")
native_platform =
  if host_os.include?("darwin") && %w[aarch64 arm64 x86_64].include?(host_cpu)
    "#{%w[aarch64 arm64].include?(host_cpu) ? "arm64" : host_cpu}-darwin"
  elsif host_os.include?("linux") && %w[aarch64 x86_64].include?(host_cpu)
    "#{host_cpu}-linux-gnu"
  else
    raise "unsupported gem platform: #{host_cpu}-#{host_os}"
  end

Gem::Specification.new do |specification|
  specification.name = "mini_vips"
  specification.version = MiniVips::VERSION
  specification.authors = ["Discourse"]
  specification.email = ["team@discourse.org"]
  specification.summary = "Fixed-purpose libvips image operations for Discourse"
  specification.homepage = "https://github.com/discourse/mini_vips"
  specification.licenses = %w[MIT OFL-1.1]
  specification.required_ruby_version = "~> 3.4.0"
  specification.required_rubygems_version = ">= 3.3.22"
  specification.platform = ENV.fetch("MINI_VIPS_PLATFORM", native_platform)

  specification.files = %w[
    CHANGELOG.md
    LICENSE
    README.md
    lib/mini_vips.rb
    lib/mini_vips/version.rb
    libexec/NotoSans-OFL.txt
    libexec/NotoSans-Regular.ttf
    libexec/mini_vips
  ]
  specification.require_paths = ["lib"]

  specification.metadata = {
    "bug_tracker_uri" => "https://github.com/discourse/mini_vips/issues",
    "changelog_uri" => "https://github.com/discourse/mini_vips/blob/main/CHANGELOG.md",
    "rubygems_mfa_required" => "true",
    "source_code_uri" => specification.homepage,
  }
end
