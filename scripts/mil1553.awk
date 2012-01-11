#
# transfer2insmod.awk - extract insmod parameters from transfer.ref
#
# usage: transfer2insmod.awk DEVICE_NAME [transfer_file]
#
# e.g.:
#  $ awk -f transfer2insmod.awk VD80 /acc/dsc/tst/cfv-864-cdv28/etc/transfer.ref
#
#  produces
#     insmod mil1553.ko bcs=1,2
#                       pci_buses=1,1
#                       pci_slots=13,14
#
# The bus number is not available from transfer.ref but by chance it is always 1
#

BEGIN	{
	device_name = ARGV[1]
	delete ARGV[1]
	bcs = ""
	pci_buses = ""
	pci_slots = ""
}

/^#\+#/ && $6 == device_name  && $4 == "PCI" {
	# decode transfer.ref line
	bcs = bcs "," $7 + 1
	pci_buses =  pci_buses "," "1"
	pci_slots =  pci_slots "," $20
}

END {

	insmod_params = " "

	if (bcs) insmod_params = insmod_params "bcs=" substr(bcs, 2)

	if (pci_buses) insmod_params = insmod_params " pci_buses=" substr(pci_buses, 2)

	if (pci_slots) insmod_params = insmod_params " pci_slots=" substr(pci_slots, 2)

	print insmod_params
}
