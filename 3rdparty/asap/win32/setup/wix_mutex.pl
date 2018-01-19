# Generate mutual exclusion conditions
use strict;
my @c = qw(wasap.ext asap_dsf.ext in_asap.ext foo_asap.ext ASAP_Apollo.ext xmp_asap.ext aimp_asap.ext libasap_plugin.ext);
my @a = map "\$$_=3 OR (\$$_=-1 AND ?$_=3)", @c; # action is install or (action is nothing and already installed)
my $i = 0;
for my $a (@a[0 .. $#a - 1]) {
	my @b = @a[++$i .. $#a];
	while (@b) {
		$_ = "(($a) AND (" . shift @b;
		while (@b) {
			my $t = "$_ OR $b[0]";
			last if length($t) > 198;
			$_ = $t;
			shift @b;
		}
		print "$_));\n";
	}
}
