import scapy
from scapy.all import *
from scapy.utils import PcapReader


def caseA(pcap):
    print("CASE A:")
    ip_addr = ""
    mac_addr = ""
    for pkg in pcap:
        if mac_addr != "":
            break
        if "DHCP" in pkg.payload and "ack" in repr(pkg.payload):
            for k,v in pkg.payload["DHCP"].options:
                if k == "router":
                    ip_addr = v
                    break
        if "ARP" in pkg.payload and pkg["ARP"].op == 2:
            if pkg["ARP"].psrc == ip_addr:
                mac_addr = pkg["ARP"].hwsrc
    print("IPAddr:", ip_addr)
    print("MACAddr:", mac_addr)

    
def caseB(pcap, website):
    print("CASE B:")
    ip_addr = ""
    for pkg in pcap:
        if "DNS" in pkg.payload and pkg["DNS"].qr == 1:
            dns = pkg["DNS"]
            if dns.an != None and website in repr(dns):
                ip_addr = dns.an.rdata
                break
    print("IPAddr-DEST:", ip_addr)
    return ip_addr
    

def caseC(pcap, ip_des):
    print("CASE C:")
    cnt = 0
    for pkg in pcap:
        if "TCP" in pkg.payload:
            tcp = pkg["TCP"]
            ip = pkg["TCP"].underlayer["IP"]
            if ip.src == ip_des or ip.dst == ip_des:
                if tcp.flags == "S" and cnt == 0:
                    cnt += 1
                    print("IPAddr-SRC:", ip.src)
                    print("IPAddr-DEST:", ip.dst)
                    print("Port-DEST:", tcp.dport)
                    print("SYN: 1\nACK: 0")
                if tcp.flags == "SA" and cnt == 1:
                    cnt += 1
                    print("IPAddr-SRC:", ip.src)
                    print("IPAddr-DEST:", ip.dst)
                    print("Port-SRC:", tcp.sport)
                    print("SYN: 1\nACK: 1")
                if tcp.flags == "A" and cnt == 2:
                    print("IPAddr-SRC:", ip.src)
                    print("IPAddr-DEST:", ip.dst)
                    print("Port-DEST:", tcp.dport)
                    print("SYN: 0\nACK: 1")
                    break

                    
    
if __name__ == '__main__':
    website = "www.google.com"
    pcap = sniff(offline="lab3_2.pcap")
    caseA(pcap)
    ip_b = caseB(pcap, website)
    caseC(pcap, ip_b)