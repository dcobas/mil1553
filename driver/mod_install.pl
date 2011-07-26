#!/usr/bin/perl
# Install CBMIA mil1553 bus controller modules from a transfer.ref 
# file and crateconfig file. 
# Based on install script of sis33 modules created by Emilio G. Cota
# and its modification for VMOD modules by Samuel Iglesias Gonsalvez
#
# first the transfer.ref file is parsed and then, after some sanity checks,
# insmod is called with the parameters extracted from transfer.ref.
#

use strict;
use warnings;

use Getopt::Long;
use Pod::Usage;

my $device = 'CBMIA';
my $driver = 'mil1553';
my $help;
my $man;
my $transferpath = '/etc/transfer.ref';
my $cratepath = '/etc/crateconfig';

my @AoH;
my @keys = ('ln', 'mln', 'bus', 'mtno', 'module-type', 'lu', 'W1', 'AM1',
	    'DPsz1', 'basaddr1', 'range1', 'W2', 'AM2', 'DPsz2', 'basaddr2',
	    'range2', 'testoff', 'sz', 'sl', 'ss');
my @cratekeys =('sl', 'pcibus', 'pcislot');
my %base_addrs;

GetOptions(
	   "help|?|h"=> \$help,
	   "device=s"=> \$device,
	   "man"=> \$man,
	   "transfer_ref=s"=> \$transferpath,
	   "crateconfig=s"=> \$cratepath,
	   ) or pod2usage(2);

pod2usage(-exitstatus => 0, -verbose => 2) if $man;

if (@ARGV > 0) {
    $driver = $ARGV[0];

    if (!defined $device) {
        $device = $driver;
    }
}

open(INPUT, "<$transferpath") or die ("$transferpath not found");
# put all the described modules in an array of hashes (AoH)
LINE: while (<INPUT>) {
    next LINE if $_ !~ m|^\#\+\#|;

    chomp;
    my @values = split(/\s+/, $_);
    # remove the first '#+#'
    shift @values;

    my $href;
    foreach (@keys) {
	$href->{$_} = shift @values;
    }

    push @AoH, $href;
    $base_addrs{$href->{'ln'}} = $href->{'basaddr1'};
}
close(INPUT);

# If we found at least a module, then the driver kernel module should be installed
# But if the vmodttl module is already there, we won't install it again
if (@AoH) {
    if (!module_is_loaded($driver)) {
	mod_install(\@AoH);
    }
} else {
    die "No $driver modules found in $transferpath.\n";
}

sub module_is_loaded {
    my $module = shift;
    
    open(LSMOD, '/proc/modules') or die('Cannot open /proc/modules.\n');
    while (<LSMOD>) {
	chomp;
	my ($modname, $rest) = split(/\s+/, $_);

	if ($modname =~ m/^$module$/) {
	    close(LSMOD);
	    return 1;
	}
    }
    close(LSMOD);
    return 0;
}

sub mod_install {
    my $AoHref = shift;
    my $index_parm;
    my $bc_num;
    my $slot;
    my $bus_number;
    my $bus;
    my $slot_number;
    my $hrefProc;
    my @AoHCrate;

open(INPUT, "<$cratepath") or die ("$cratepath not found");
while (<INPUT>) {
    chomp;
     my @values = split(/\s+/, $_);
     my $hrefcrate;

    foreach (@cratekeys) {
	$hrefcrate->{$_} = shift @values;
    }

    push @AoHCrate, $hrefcrate;
}
close(INPUT);

  ENTRY: foreach my $href (@{$AoHref}) {
      next ENTRY if $href->{'module-type'} !~ m/$device/;
      
      if ($href->{'sl'} <= 0) {
	  die("Slot $href->{'sl'} is forbidden. There is an error in the configuration.\n");
      }

    CRATE_ENTRY: foreach my $crate (@AoHCrate) {
      next CRATE_ENTRY if $crate->{'sl'} != $href->{'sl'};

      $bus = $crate->{'pcibus'};
      $slot = $crate->{'pcislot'};
      last if(1);
  }

      if (defined $index_parm) {
	  $index_parm = $index_parm.','.$href->{'lu'};
	  $bus_number = $bus_number.','.'0x'.$bus;
	  $slot_number = $slot_number.','.'0x'.$slot;
      } else {
	  $index_parm = $href->{'lu'};
	  $bus_number = '0x'.$bus;
	  $slot_number = '0x'.$slot;
      }
      $bus = -1;
      $slot = -1;
	
  }
    if (defined $index_parm) {
	my $driver_name = $driver.".ko";
	my $bc_num = $index_parm + 1;
	my $insmod = "insmod $driver_name bc_num=$bc_num bus_number=$bus_number slot_number=$slot_number";
	print $insmod, "\n";
	system($insmod) == 0 or die("$insmod failed");
	
    }
}

__END__

=head1 NAME

mod_install.pl - Install the PCI driver kernel module using transfer.ref file.

=head1 SYNOPSIS

mod_install.pl [OPTIONS] driver

mod_install.pl - Install the PCI driver kernel module using B<transfer.ref> file.

=head1 OPTIONS

=over 8

=item B<--device>

Name of the device(s) controlled by B<driver>. The default value is that of B<driver>.

=item B<--help -h -?>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<--transfer_ref>=FILE

Filename of a transfer.ref alternative to the default '/etc/transfer.ref'.

=back

=head1 DESCRIPTION

B<mod_install.pl> parses a B<transfer.ref> file (taken by default from '/etc/transfer.ref')
and install it using the information gets from this file. 
Interrupt vectors, which are available in the transfer.ref file in the install instructions, 
are passed as arguments to the B<VMOD-like> kernel module.

By default all of B<VMOD-like> devices in the transfer.ref are processed.
Each irq provided is assigned to each device in the
same order as it appears in the transfer.ref file.

=head1 AUTHOR

Written by Samuel I. Gonsalvez

=cut
