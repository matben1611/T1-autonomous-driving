#!/usr/bin/zsh
# get with `lspci -D`
echo -n "0000:09:00.1" > /sys/bus/pci/drivers/xhci_hcd/unbind
echo -n "0000:09:00.1" > /sys/bus/pci/drivers/xhci_hcd/bind
