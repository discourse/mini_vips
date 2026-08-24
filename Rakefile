# frozen_string_literal: true

require "fileutils"
require "rbconfig"
require "rubygems/package"
require "rake/testtask"
require_relative "lib/mini_vips"

platforms = %w[aarch64-linux-gnu arm64-darwin x86_64-linux-gnu].freeze
host_cpu = RbConfig::CONFIG.fetch("host_cpu")
host_os = RbConfig::CONFIG.fetch("host_os")
local_platform =
  if host_os.include?("darwin") && %w[aarch64 arm64].include?(host_cpu)
    "arm64-darwin"
  elsif host_os.include?("linux") && %w[aarch64 x86_64].include?(host_cpu)
    "#{host_cpu}-linux-gnu"
  else
    abort "unsupported build platform: #{host_cpu}-#{host_os}"
  end

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
  platform = ENV.fetch("MINI_VIPS_PLATFORM", local_platform)
  supported_platform = platforms.include?(platform)
  abort "unsupported MINI_VIPS_PLATFORM: #{platform}" unless supported_platform

  ENV["MINI_VIPS_PLATFORM"] = platform
  FileUtils.mkdir_p("pkg")
  specification = Gem::Specification.load("mini_vips.gemspec")
  gem_file = Gem::Package.build(specification)
  FileUtils.mv(gem_file, File.join("pkg", gem_file))
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
