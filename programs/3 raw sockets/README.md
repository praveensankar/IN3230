
Usage:

- Compile all programmes with 'make all' in the current directory
- Create the mininet topology using 'sudo mn --mac --custom topo_p2p.py --topo mytopo'
- In the mininet console, access node h1 and h2 using 'xterm h1' and 'xterm h2' or 'xterm h1 h2'
  (You should have used -Y argument in the ssh command: ssh -Y debian@ip_address_of_your_VM
- From the xterm consoles, run ./receiver at node h2 and ./sender at node h1

