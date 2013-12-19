#!/usr/bin/perl

use strict;

my $Version = shift;
my @PackageNames = split /\s/, shift;
my %Packages = ();

if (${Version} eq '')
{
  print "Detect version\n";
  $Version = `make -s version_name -C apps/version` or die 'Failed to determine version';
  chomp ${Version};
}

die "Invalid version '${Version}'" unless ${Version} =~ /^r(\d+)(-\d+-g[0-9a-f]+)?M?$/;

my $Revision = $1;

if (!scalar(@PackageNames))
{
  print "Using default packages set\n";
  @PackageNames = qw(xtractor zxtune123 zxtune-qt zxtune);
}

for my $subfolder (qw(windows/x86 windows/x86_64 mingw/x86 mingw/x86_64 linux/i686 linux/x86_64 linux/arm linux/armhf dingux/mipsel android))
{
  my $folder = "Builds/${Version}/${subfolder}";
  next unless -d ${folder};
  my ($platform, $arch) = split('/',${subfolder});
  print "Platform: ${platform} arch: ${arch}\n";
  for my $package (@PackageNames)
  {
    for my $file (glob("${folder}/${package}*"))
    {
      my $opsys = undef;
      my $compiler = 'gcc';
      my $type = 'Archive';
      if (${file} =~ /${package}_${Version}_${platform}_${arch}\./)
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
      elsif (${file} =~ /${package}-${Version}-\d+-${arch}\.pkg\.tar\.xz/)
      {
        $opsys = 'Archlinux';
        $type = 'Package';
      }
      elsif (${file} =~ /${package}_${Version}_.*?\.deb/)
      {
        $opsys = 'Ubuntu';
        $type = 'Package';
      }
      elsif (${file} =~ /${package}-${Version}-\d+(.*)?\.${arch}\.rpm/)
      {
        $opsys = 'Redhat';
        $type = 'Package';
      }
      elsif (${file} =~ /${package}_${Version}.apk/)
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
