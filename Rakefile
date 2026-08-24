# frozen_string_literal: true

require "fileutils"
require "bundler"
require "rbconfig"
require "rubygems/package"
require "rake/testtask"
require "tmpdir"

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
gem_version = -> { Gem::Specification.load("mini_vips.gemspec").version }

Rake::TestTask.new do |task|
  task.libs << "lib" unless ENV["MINI_VIPS_INSTALLED_TEST"] == "1"
  task.libs << "test"
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
      gem_file = File.join("pkg", "mini_vips-#{gem_version.call}-#{build_platform.call}.gem")
      package_directory = File.join(install_directory, "mini_vips")
      package = Gem::Package.new(gem_file)
      package.extract_files(package_directory)
      File.write(File.join(package_directory, "mini_vips.gemspec"), package.spec.to_ruby)
      gemfile = File.join(install_directory, "Gemfile")
      File.write(gemfile, <<~GEMFILE)
          source "https://rubygems.org"

          gem "chunky_png"
          gem "minitest"
          gem "mini_vips", path: #{package_directory.inspect}
          gem "rake"
        GEMFILE
      bundle_environment = {
        "BUNDLE_GEMFILE" => gemfile,
        "BUNDLE_IGNORE_CONFIG" => "1",
        "BUNDLE_PATH" => File.join(install_directory, "bundle"),
        "MINI_VIPS_INSTALLED_TEST" => "1",
      }

      Bundler.with_unbundled_env do
        sh bundle_environment, "bundle", "install"
        sh bundle_environment,
           "bundle",
           "exec",
           "rake",
           "--rakefile",
           File.expand_path("Rakefile"),
           "test"
      end
    end
  end
end

desc "Publish platform gems"
task :release do
  Dir["pkg/*.gem"].sort.each { |gem_file| sh "gem", "push", gem_file }
  tag = "v#{gem_version.call}"
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
