# frozen_string_literal: true

require_relative "lib/mini_vips/version"

Gem::Specification.new do |specification|
  specification.name = "mini_vips"
  specification.version = MiniVips::VERSION
  specification.authors = ["Discourse"]
  specification.email = ["team@discourse.org"]
  specification.summary = "Small, fixed-purpose image operations powered by libvips"
  specification.homepage = "https://github.com/discourse/mini_vips"
  specification.license = "MIT"
  specification.required_ruby_version = ">= 3.2"
  specification.required_rubygems_version = ">= 3.3.22"
  specification.platform = ENV.fetch("MINI_VIPS_PLATFORM", Gem::Platform.local.to_s)

  specification.files = %w[
    LICENSE
    README.md
    lib/mini_vips.rb
    lib/mini_vips/version.rb
    libexec/mini_vips
  ]
  specification.require_paths = ["lib"]

  specification.metadata = {
    "bug_tracker_uri" => "https://github.com/discourse/mini_vips/issues",
    "changelog_uri" => "https://github.com/discourse/mini_vips/releases",
    "rubygems_mfa_required" => "true",
    "source_code_uri" => specification.homepage,
  }
end
