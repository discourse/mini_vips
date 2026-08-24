# frozen_string_literal: true

require "fileutils"
require "digest"
require "json"
require "net/http"
require "rubygems/package"
require "rake/testtask"
require "uri"
require_relative "lib/mini_vips"

platforms = %w[aarch64-linux-gnu arm64-darwin x86_64-linux-gnu].freeze

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
  platform = ENV.fetch("MINI_VIPS_PLATFORM")
  supported_platform = platforms.include?(platform)
  abort "unsupported MINI_VIPS_PLATFORM: #{platform}" unless supported_platform

  FileUtils.mkdir_p("pkg")
  specification = Gem::Specification.load("mini_vips.gemspec")
  gem_file = Gem::Package.build(specification)
  FileUtils.mv(gem_file, File.join("pkg", gem_file))
end

namespace :release do
  desc "Publish every platform gem for the current version"
  task :platforms do
    version = MiniVips::VERSION
    expected_gems =
      platforms.to_h { |platform| [platform, "pkg/mini_vips-#{version}-#{platform}.gem"] }
    expected_files = expected_gems.values
    actual_files = Dir["pkg/*.gem"].sort
    unless actual_files == expected_files.sort
      abort "expected #{expected_files.sort.join(", ")}, found #{actual_files.join(", ")}"
    end

    artifacts =
      expected_gems.map do |platform, gem_file|
        specification = Gem::Package.new(gem_file).spec
        unless specification.name == "mini_vips" && specification.version.to_s == version &&
                 specification.platform.to_s == platform
          abort "unexpected gem in #{gem_file}"
        end
        {
          platform: platform,
          gem_file: gem_file,
          checksum: Digest::SHA256.file(gem_file).hexdigest,
        }
      end

    versions_uri = URI("https://rubygems.org/api/v1/versions/mini_vips.json")
    fetch_versions =
      lambda do
        response = Net::HTTP.get_response(versions_uri)
        next [] if response.is_a?(Net::HTTPNotFound)
        unless response.is_a?(Net::HTTPSuccess)
          abort "RubyGems versions API returned #{response.code}"
        end

        JSON.parse(response.body)
      end

    artifacts.each do |artifact|
      published =
        fetch_versions.call.find do |candidate|
          candidate.fetch("number") == version &&
            candidate.fetch("platform") == artifact.fetch(:platform)
        end

      if published
        unless published.fetch("sha") == artifact.fetch(:checksum)
          abort "published checksum differs for #{artifact.fetch(:platform)}"
        end
        next
      end

      sh "gem", "push", artifact.fetch(:gem_file)
    end

    confirmed = false
    10.times do
      published_versions = fetch_versions.call
      confirmed =
        artifacts.all? do |artifact|
          published_versions.any? do |candidate|
            candidate.fetch("number") == version &&
              candidate.fetch("platform") == artifact.fetch(:platform) &&
              candidate.fetch("sha") == artifact.fetch(:checksum)
          end
        end
      break if confirmed

      sleep 3
    end
    abort "RubyGems did not confirm every platform gem" unless confirmed

    sh "git", "tag", "v#{version}"
    sh "git", "push", "origin", "v#{version}"
  end
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
