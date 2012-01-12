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

BEGIN {
	device_name = ARGV[1]
	delete ARGV[1]
	crateconfig = ARGV[2]
	delete ARGV[2]
	while (getline <crateconfig > 0) {
		slot_to_pci_bus[$1] = $2
		slot_to_pci_slot[$1] = "0x" $3
	}
	bcs = ""
	slots = ""
}

/^#\+#/ && $6 == device_name  && $4 == "PCI" {
	# decode transfer.ref line
	bcs = bcs "," $7 + 1
	slots =  slots "," $20
	pci_bus  = pci_bus "," slot_to_pci_bus[$20]
	pci_slot = pci_slot "," slot_to_pci_slot[$20]
}

END {
	insmod_params = " "

	if (bcs)
		insmod_params = insmod_params "bcs=" substr(bcs, 2)
	if (pci_bus) {
		insmod_params = insmod_params " pci_buses=" substr(pci_bus, 2)
		insmod_params = insmod_params " pci_slots=" substr(pci_slot, 2)
	}
	print insmod_params
}
