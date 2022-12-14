# This script was automatically generated from the dsa-883
# Debian Security Advisory
# It is released under the Nessus Script Licence.
# Advisory is copyright 1997-2004 Software in the Public Interest, Inc.
# See http://www.debian.org/license
# DSA2nasl Convertor is copyright 2004 Michel Arboi

if (! defined_func('bn_random')) exit(0);

desc = '
Javier Fern?ndez-Sanguino Pe?a from the Debian Security Audit team
discovered that the syslogtocern script from thttpd, a tiny webserver,
uses a temporary file insecurely, allowing a local attacker to craft a
symlink attack to overwrite arbitrary files.
For the old stable distribution (woody) this problem has been fixed in
version 2.21b-11.3.
For the stable distribution (sarge) this problem has been fixed in
version 2.23beta1-3sarge1.
For the unstable distribution (sid) this problem has been fixed in
version 2.23beta1-4.
We recommend that you upgrade your thttpd package.


Solution : http://www.debian.org/security/2005/dsa-883
Risk factor : High';

if (description) {
 script_id(22749);
 script_version("$Revision: 1.1 $");
 script_xref(name: "DSA", value: "883");
 script_cve_id("CVE-2005-3124");

 script_description(english: desc);
 script_copyright(english: "This script is (C) 2006 Michel Arboi <mikhail@nessus.org>");
 script_name(english: "[DSA883] DSA-883-1 thttpd");
 script_category(ACT_GATHER_INFO);
 script_family(english: "Debian Local Security Checks");
 script_dependencies("ssh_get_info.nasl");
 script_require_keys("Host/Debian/dpkg-l");
 script_summary(english: "DSA-883-1 thttpd");
 exit(0);
}

include("debian_package.inc");

w = 0;
if (deb_check(prefix: 'thttpd', release: '', reference: '2.23beta1-4')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd is vulnerable in Debian .\nUpgrade to thttpd_2.23beta1-4\n');
}
if (deb_check(prefix: 'thttpd', release: '3.0', reference: '2.21b-11.3')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd is vulnerable in Debian 3.0.\nUpgrade to thttpd_2.21b-11.3\n');
}
if (deb_check(prefix: 'thttpd-util', release: '3.0', reference: '2.21b-11.3')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd-util is vulnerable in Debian 3.0.\nUpgrade to thttpd-util_2.21b-11.3\n');
}
if (deb_check(prefix: 'thttpd', release: '3.1', reference: '2.23beta1-3sarge1')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd is vulnerable in Debian 3.1.\nUpgrade to thttpd_2.23beta1-3sarge1\n');
}
if (deb_check(prefix: 'thttpd-util', release: '3.1', reference: '2.23beta1-3sarge1')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd-util is vulnerable in Debian 3.1.\nUpgrade to thttpd-util_2.23beta1-3sarge1\n');
}
if (deb_check(prefix: 'thttpd', release: '3.1', reference: '2.23beta1-3sarge1')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd is vulnerable in Debian sarge.\nUpgrade to thttpd_2.23beta1-3sarge1\n');
}
if (deb_check(prefix: 'thttpd', release: '3.0', reference: '2.21b-11.3')) {
 w ++;
 if (report_verbosity > 0) desc = strcat(desc, '\nThe package thttpd is vulnerable in Debian woody.\nUpgrade to thttpd_2.21b-11.3\n');
}
if (w) { security_hole(port: 0, data: desc); }
