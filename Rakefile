# frozen_string_literal: true

require "fileutils"
require "bundler"
require "rbconfig"
require "rubygems/package"
require "rake/testtask"
require "tmpdir"
require_relative "lib/mini_vips"

platforms = %w[aarch64-linux-gnu arm64-darwin x86_64-darwin x86_64-linux-gnu].freeze
host_cpu = RbConfig::CONFIG.fetch("host_cpu")
host_os = RbConfig::CONFIG.fetch("host_os")
local_platform =
  if host_os.include?("darwin") && %w[aarch64 arm64 x86_64].include?(host_cpu)
    "#{%w[aarch64 arm64].include?(host_cpu) ? "arm64" : host_cpu}-darwin"
  elsif host_os.include?("linux") && %w[aarch64 x86_64].include?(host_cpu)
    "#{host_cpu}-linux-gnu"
  else
    abort "unsupported build platform: #{host_cpu}-#{host_os}"
  end
build_platform = -> { ENV.fetch("MINI_VIPS_PLATFORM", local_platform) }

Rake::TestTask.new do |task|
  task.libs << "lib" << "test"
  task.pattern = "test/**/*_test.rb"
end

desc "Compile the native helper"
task :compile do
  sh "script/build"
end

desc "Build a platform gem"
task build: :compile do
  platform = build_platform.call
  supported_platform = platforms.include?(platform)
  abort "unsupported MINI_VIPS_PLATFORM: #{platform}" unless supported_platform

  FileUtils.mkdir_p("pkg")
  original_platform = ENV["MINI_VIPS_PLATFORM"]
  specification =
    begin
      ENV["MINI_VIPS_PLATFORM"] = platform
      Gem::Specification.load("mini_vips.gemspec")
    ensure
      if original_platform
        ENV["MINI_VIPS_PLATFORM"] = original_platform
      else
        ENV.delete("MINI_VIPS_PLATFORM")
      end
    end
  gem_file = Gem::Package.build(specification)
  FileUtils.mv(gem_file, File.join("pkg", gem_file))
end

namespace :test do
  desc "Test the installed platform gem"
  task installed: :build do
    Dir.mktmpdir do |install_directory|
      gem_file = File.join("pkg", "mini_vips-#{MiniVips::VERSION}-#{build_platform.call}.gem")
      test_require_paths =
        %w[chunky_png minitest].flat_map do |gem_name|
          Gem.loaded_specs.fetch(gem_name).full_require_paths
        end
      Bundler.with_unbundled_env do
        sh(
          "gem",
          "install",
          "--install-dir",
          install_directory,
          "--local",
          gem_file,
          "--no-document",
        )
        sh(
          { "GEM_HOME" => install_directory, "GEM_PATH" => install_directory },
          RbConfig.ruby,
          "-I#{test_require_paths.join(File::PATH_SEPARATOR)}",
          "test/all_tests.rb",
        )
      end
    end
  end
end

desc "Publish platform gems"
task :release do
  Dir["pkg/*.gem"].sort.each { |gem_file| sh "gem", "push", gem_file }
  tag = "v#{MiniVips::VERSION}"
  sh "git", "tag", tag
  sh "git", "push", "origin", tag
end

desc "Run Ruby tests and style checks"
task default: %i[compile test rubocop stree]

task :rubocop do
  sh "bundle", "exec", "rubocop"
end

task :stree do
  ruby_files = FileList["**/*.rb"].exclude("vendor/**/*")
  sh "bundle", "exec", "stree", "check", "Gemfile", "Rakefile", "mini_vips.gemspec", *ruby_files
end
