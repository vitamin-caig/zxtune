#!/usr/bin/perl

use strict;

my $Revision = shift;
my @PackageNames = split /\s/, shift;
my %Packages = ();

if (${Revision} eq '')
{
  print "Detect last revision\n";
  $Revision = `svnversion` or die 'Failed to determine version';
  chomp ${Revision};
}

die "Invalid revision ${Revision}" unless ${Revision} =~ /^\d+M?$/;

if (!scalar(@PackageNames))
{
  print "Using default packages set\n";
  @PackageNames = qw(xtractor zxtune123 zxtune-qt zxtune);
}

for my $folder (glob("Builds/Revision${Revision}_*"))
{
  die "Invalid folder format (${folder})" unless ${folder} =~ /Revision${Revision}_([^_]+)(_(.*))?/;
  die 'Not a folder' unless -d ${folder};
  my $platform = $1;
  my $arch = $3;
  for my $package (@PackageNames)
  {
    for my $file (glob("${folder}/${package}*"))
    {
      my $opsys = undef;
      my $compiler = 'gcc';
      my $type = 'Archive';
      if (${file} =~ /${package}_r${Revision}_${platform}_${arch}\./)
      {
        if (${platform} eq 'windows')
        {
          $opsys = 'Windows';
          $compiler = ${arch} eq 'x86_64' ? 'vc80' : 'vc71';
        }
        elsif (${platform} eq 'mingw')
        {
          $opsys = 'Windows';
        }
        elsif (${platform} eq 'linux')
        {
          $opsys = 'Linux';
        }
        elsif (${platform} eq 'dingux')
        {
          $opsys = 'Dingux';
        }
        else
        {
          die 'Unknown platform '.${platform};
        }
      }
      elsif (${file} =~ /${package}-r${Revision}-\d+-${arch}\.pkg\.tar\.xz/)
      {
        $opsys = 'Archlinux';
        $type = 'Package';
      }
      elsif (${file} =~ /${package}_r${Revision}_.*?\.deb/)
      {
        $opsys = 'Ubuntu';
        $type = 'Package';
      }
      elsif (${file} =~ /${package}-r${Revision}-\d+(.*)?\.${arch}\.rpm/)
      {
        $opsys = 'Redhat';
        $type = 'Package';
      }
      elsif (${file} =~ /${package}_r${Revision}.apk/)
      {
        $opsys = 'Android';
        $type = 'Package';
        $arch = 'any';
      }
      else
      {
        #print "Skipping ${file}\n";
        next;
      }
      my $normalized_arch = ${arch} eq 'i686' ? 'x86' : ${arch};
      $Packages{${package}}->{${opsys}}->{${normalized_arch}}->{${compiler}} = {'File' => ${file}, 'Type' => ${type}};
      print "Found '${file}' (${package} ${opsys} ${normalized_arch} ${compiler} ${type})\n";
    }
  }
}

open ACCFILE, '<../account' or die 'No accound file';
my $account = <ACCFILE>;
chomp $account;
my ($project, $user, $password) = split(/:/, ${account});

print "Continue? ";
my $answer = <>;
chomp $answer;
exit 0 unless ${answer} eq 'yes';

for my $opsys qw(Redhat Ubuntu Archlinux Dingux Linux Windows Android)
{
  for my $compiler qw(gcc vc80 vc71)
  {
    for my $arch qw(mipsel armhf arm x86_64 x86 any)
    {
      for my $package (@PackageNames)
      {
        my $key = $Packages{${package}}->{${opsys}}->{${arch}}->{${compiler}};
        next unless defined $key;
        my $file = $key->{'File'};
        my $type = $key->{'Type'};
        my $tags = "Type-${type},OpSys-${opsys},Platform-${arch},Compiler-${compiler}";
        print "Uploading '${file}' (${tags})\n";
        die unless -e ${file};
        system 'python', 'make/build/googlecode_upload.py',
               '-s', "${package} revision ${Revision}",
               '-p', ${project},
               '-u', ${user},
               '-w', ${password},
               '-l', "${tags}",
               ${file};
        die 'Failed' if $_ / 256;
      }
    }
  }
}
