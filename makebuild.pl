#!/usr/bin/perl

my $distribution;

sub detect_platform
{
  #check for linux
  return ('linux', `arch`) if -e '/proc/cmdline';
  return ('windows', 'win32');
}

sub target_dir
{
  {
    open FILE, 'svn up|' or die 'Failed to update.';
    while (<FILE>)
    {
      $revision = $1 if (/revision\s*(\d+)/);
    }
  }
  die 'Failed to determine current revision' unless defined $revision;
  my $changes = 0;
  {
    open FILE, 'svn -q st|' or die 'Failed to stat.';
    while (<FILE>)
    {
      ++$changes if (/^[ACDMR\!]/);
    }
  }
  $revision = sprintf '%04d', $revision;
  $revision .= '-devel' if $changes;
  my $dirname = 'Builds/Revision'.$revision;
  ($dirname, $revision);
}

my ($platform, $arch) = scalar @ARGV >= 2 ? (shift, shift) : detect_platform();
chomp ($platform, $arch);
print "Building $platform platform for architecture $arch.\nCleanup environment.\n";
my ($dirname, $revision) = target_dir();
system "rm -Rf bin lib obj ${dirname}";

print "Building...\n";
system("make -j 2 -C apps/zxtune123 mode=release platform=$platform defines=ZXTUNE_VERSION=rev${revision} > build.log 2>&1") / 256 == 0 or die 'Failed to build.';

mkdir $dirname or die 'Failed to create build directory';
my $zipfile = "$dirname/zxtune123_r${revision}_${arch}.zip";
print "Storing build to $zipfile.\n";
system("zip -9Dj $zipfile bin/${platform}/release/zxtune* -x bin/${platform}/release/\\*.pdb") / 256 == 0 or die 'Failed to pack binary';
if ($distribution)
{
  system("zip -9D $zipfile $distribution") / 256 == 0 or die 'Failed to pack supplementary';
}
system("cp build.log bin/${platform}/release/*.pdb $dirname") / 256 == 0 or die 'Failed to move additional files';
print "Done!\n";
