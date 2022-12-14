# This plugin was written from scratch by Michel Arboi <arboi@alussinan.org>
# It is released under the GNU Public Licence (GPLv2)
#
# References:
# RFC 1058	Routing Information Protocol
# RFC 2453	RIP Version 2
#
#
#      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
#     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#     | command (1)   | version (1)   |      must be zero (2)         |
#     +---------------+---------------+-------------------------------+
#     | address family identifier (2) |      must be zero (2)         |
#     +-------------------------------+-------------------------------+
#     |                         IP address (4)                        |
#     +---------------------------------------------------------------+
#     |                        must be zero (4) (netmask with RIP-2)  |
#     +---------------------------------------------------------------+
#     |                        must be zero (4) (next hop in RIP-2)   |
#     +---------------------------------------------------------------+
#     |                          metric (4)                           |
#     +---------------------------------------------------------------+
#
#  1 - request     A request for the responding system to send all or
#                  part of its routing table.
#
#  2 - response    A message containing all or part of the sender's
#                  routing table.  This message may be sent in response
#                  to a request or poll, or it may be an update message
#                  generated by the sender.
#
#  3 - traceon     Obsolete.  Messages containing this command are to be
#                  ignored.
#
#  4 - traceoff    Obsolete.  Messages containing this command are to be
#                  ignored.
#
#  5 - reserved    This value is used by Sun Microsystems for its own
#                  purposes.  If new commands are added in any
#                  succeeding version, they should begin with 6.
#                  Messages containing this command may safely be
#                  ignored by implementations that do not choose to
#                  respond to it.
#

if(description)
{
  script_id(11822);
  script_version ("$Revision: 1.20 $");

  name["english"] = "RIP detection";
  script_name(english:name["english"]);
 
  desc["english"] = "
This plugin detects RIP-1 and RIP-2 agents and display 
their routing tables.

Risk factor : Low";

  script_description(english:desc["english"]);
 
  summary["english"] = "RIP server detection";
  script_summary(english:summary["english"]);
  script_category(ACT_GATHER_INFO); 
  script_copyright(english:"This script is Copyright (C) 2003 Michel Arboi");
  script_family(english:"Service detection");
  exit(0);
}

##include("dump.inc");
include('global_settings.inc');
include("network_func.inc");
include("misc_func.inc");

function rip_test(port, priv)
{
  local_var	soc, req, r, l, ver, report, i, n, ip_addr, mask, metric, next_hop, kbd, fam;

if (priv)
  soc = open_priv_sock_udp(dport:port, sport:port);
else
  soc = open_sock_udp(port);

if (!soc) return(0);

# Special request - See ?3.4.1 of RFC 1058

r = "";
for (v = 2; v >= 1 && strlen(r) == 0; v --)
{
  req = raw_string(1, v, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 16);
  send(socket: soc, data: req);
  ##dump(ddata: r, dtitle: "routed");
}
r = recv(socket:soc, length: 512, timeout:3);
close(soc);

l = strlen(r);
if (l < 4 || ord(r[0]) != 2) return(0);	# Not a RIP answer
ver = ord(r[1]); 
if (ver != 1 && ver != 2) return(0);	# Not a supported RIP version?

set_kb_item(name: "/rip/" + port + "/version", value: ver);

report = strcat('A RIP-', ver, ' agent is running on this port.\n');
n = 0;
for (i = 4; i < l; i += 20)
{
  fam = 256 * ord(r[i]) + ord(r[i+1]);
  if (fam == 2)
  {
    ip_addr = strcat(ord(r[i+4]), ".", ord(r[i+5]), ".", ord(r[i+6]), ".", ord(r[i+7]));
    mask = strcat(ord(r[i+8]), ".", ord(r[i+9]), ".", ord(r[i+10]), ".", ord(r[i+11]));
    nexthop = strcat(ord(r[i+12]), ".", ord(r[i+13]), ".", ord(r[i+14]), ".", ord(r[i+15]));
    metric = ord(r[i+19]) + 256 * (ord(r[i+18]) + 256 * (ord(r[i+17]) + 256 * ord(r[i+16])));
    if (n == 0) report += 'The following routes are advertised:\n';
    n ++;

    kbd = strcat('/routes/',n);
    set_kb_item(name: kbd + '/addr', value: ip_addr);

    if (ver == 1)
      report += ip_addr;
    else
    {
      report = strcat(report, ip_addr, '/', mask);
      set_kb_item(name: kbd + '/mask', value: mask);
    }

    if (metric == 16)
      report += ' at infinity';
    else if (metric <= 1)
      report = strcat(report, ' at ', metric, ' hop');
    else
      report = strcat(report, ' at ', metric, ' hops');
    set_kb_item(name: kbd + '/metric', value: metric);
    if (ver > 1 && nexthop != '0.0.0.0')
    {
      report = strcat(report, ', next hop at ', nexthop);
      set_kb_item(name: kbd + '/nexthop', value:nexthop);
    }
    report += '\n';
  }
  else
  {
    display("Unknown address family: ", fam, '\n');
  }
}

if (n > 0)
  report += 'This information on your network topology may help an attacker \n\nRisk factor : Low';
else
  report += '\nRisk factor: None';

security_note(port: port, data: report, protocol: "udp");
register_service(port: port, ipproto: "udp", proto: "rip");

# Remember that a machine may have to route packets even if it only 
# has one interface!

if (!is_private_addr())
  security_hole(port: port, protocol: "udp", data: 
'Running RIP on Internet is definitely a bad idea, as this "IGP" 
routing protocol is neither efficient nor secure for wide area networks.

Solution: disable the RIP agent and use an "EGP" routing protocol
Risk factor: High');
else
  if (ver == 1)
    security_warning(port: port, protocol: "udp", data: 
'RIP-1 does not implement authentication. 
An attacker may feed your machine with bogus routes and
hijack network connections.

Solution : disable the RIP agent if you don\'t use it, or use
           RIP-2 and implement authentication
Risk factor : Medium');
  else # RIP-2
    if (! islocalnet())	# rip_poison will not be able to test the security
      security_note(port: port, protocol: "udp", data: 
'RIP-2 allows authentication but Nessus has no fully reliable way 
to check if it was properly implemented. 
If not, an attacker may feed your machine with bogus routes and
hijack network connections.

Solution : implement RIP-2 authentication if necessary or 
           disable the RIP agent if you don\'t use it.
Risk factor : Low / Medium');
return(1);
}

port = 520;
#if (! get_udp_port_state(port)) exit(0); # Not very efficient with UDP!

if (rip_test(port: port, priv: 0)) exit(0);
if (rip_test(port: port, priv: 1))
{
  security_note(port: port, protocol: "udp", data: "
This RIP agent is broken: it only answers to requests where the source
port is set to 520.
This is not RFC compliant, but does not have security consequences.

Risk : None");
  set_kb_item(name: "/rip/" + port + "/broken_source_port", value: TRUE);
}
