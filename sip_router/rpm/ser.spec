%define name    ser
%define ver     0.8.11pre29
%define rel     0

%define EXCLUDED_MODULES	mysql jabber auth_radius group_radius uri_radius \
							postgress snmp cpl cpl-c ext extcmd mangler pdt
%define MYSQL_MODULES		mysql
%define JABBER_MODULES		jabber
%define RADIUS_MODULES		auth_radius group_radius uri_radius
%define RADIUS_MOD_PATH		modules/auth_radius modules/group_radius \
							modules/uri_radius

Summary:      SIP Express Router, very fast and flexible SIP Proxy
Name:         %name
Version:      %ver
Release:      %rel
Packager:     Jan Janak <jan@iptel.org>
Copyright:    GPL
Group:        System Environment/Daemons
Source:       http://iptel.org/ser/stable/%{name}-%{ver}_src.tar.gz
Source2:      ser.init
URL:          http://iptel.org/ser
Vendor:       FhG Fokus
BuildRoot:    /var/tmp/%{name}-%{ver}-root
BuildPrereq:  make flex bison


%description
Ser or SIP Express Router is a very fast and flexible SIP (RFC3621)
proxy server. Written entirely in C, ser can handle thousands calls
per second even on low-budget hardware. A C Shell like scripting language
provides full control over the server's behaviour. It's modular
architecture allows only required functionality to be loaded.
Currently the following modules are available: digest authentication,
CPL scripts, instant messaging, MySQL support, a presence agent, radius
authentication, record routing, an SMS gateway, a jabber gateway, a 
transaction module, registrar and user location.

%package  mysql
Summary:  MySQL connectivity for the SIP Express Router.
Group:    System Environment/Daemons
Requires: ser
BuildPrereq:  mysql-devel zlib-devel

%description mysql
The ser-mysql package contains MySQL database connectivity that you
need to use digest authentication module or persistent user location
entries.

%package  jabber
Summary:  sip jabber message translation support for the SIP Express Router.
Group:    System Environment/Daemons
Requires: ser
BuildPrereq:  expat-devel

%description jabber
The ser-jabber package contains a sip to jabber message translator.

%package  radius
Summary:  ser radius authentication, group and uri check modules.
Group:    System Environment/Daemons
Requires: ser
BuildPrereq:  libradius1-dev

%description radius
The ser-radius package contains modules for radius authentication, group
 membership and uri checking.

%prep
%setup

%build
make all skip_modules="%EXCLUDED_MODULES"      cfg-target=/%{_sysconfdir}/ser/
make modules modules="modules/%MYSQL_MODULES"  cfg-target=/%{_sysconfdir}/ser/
make modules modules="modules/%JABBER_MODULES" cfg-target=/%{_sysconfdir}/ser/
make modules modules="%RADIUS_MOD_PATH"        cfg-target=/%{_sysconfdir}/ser/


%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

make install skip_modules="%EXCLUDED_MODULES" \
		basedir=$RPM_BUILD_ROOT \
		prefix=/usr \
		cfg-prefix=$RPM_BUILD_ROOT/%{_sysconfdir} \
		cfg-target=/%{_sysconfdir}/ser/ 
make install-modules modules="modules/%MYSQL_MODULES" \
		basedir=$RPM_BUILD_ROOT \
		prefix=/usr \
		cfg-prefix=$RPM_BUILD_ROOT/%{_sysconfdir} \
		cfg-target=/%{_sysconfdir}/ser/ 
make install-modules modules="modules/%JABBER_MODULES" \
		basedir=$RPM_BUILD_ROOT \
		prefix=/usr \
		cfg-prefix=$RPM_BUILD_ROOT/%{_sysconfdir} \
		cfg-target=/%{_sysconfdir}/ser/ 
make install-modules modules="%RADIUS_MOD_PATH" \
		basedir=$RPM_BUILD_ROOT \
		prefix=/usr \
		cfg-prefix=$RPM_BUILD_ROOT/%{_sysconfdir} \
		cfg-target=/%{_sysconfdir}/ser/ 

mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d
install -m755 $RPM_SOURCE_DIR/ser.init \
              $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/ser


mkdir -p $RPM_BUILD_ROOT/%{_bindir}
install -m755 scripts/harv_ser.sh \
              $RPM_BUILD_ROOT/%{_bindir}/harv_ser.sh

mv $RPM_BUILD_ROOT/%{_sbindir}/gen_ha1 $RPM_BUILD_ROOT/%{_bindir}


%clean
rm -rf "$RPM_BUILD_ROOT"

%post
/sbin/chkconfig --add ser

%preun
if [ $1 = 0 ]; then
    /sbin/service ser stop > /dev/null 2>&1
    /sbin/chkconfig --del ser
fi


%files
%defattr(-,root,root)
%doc README

%dir %{_sysconfdir}/ser
%config(noreplace) %{_sysconfdir}/ser/*
%config %{_sysconfdir}/rc.d/init.d/*

%dir %{_libdir}/ser
%dir %{_libdir}/ser/modules
%{_libdir}/ser/modules/acc.so
%{_libdir}/ser/modules/auth.so
%{_libdir}/ser/modules/auth_db.so
%{_libdir}/ser/modules/dbtext.so
%{_libdir}/ser/modules/domain.so
%{_libdir}/ser/modules/enum.so
%{_libdir}/ser/modules/exec.so
%{_libdir}/ser/modules/group.so
%{_libdir}/ser/modules/maxfwd.so
%{_libdir}/ser/modules/msilo.so
%{_libdir}/ser/modules/nathelper.so
%{_libdir}/ser/modules/pa.so
%{_libdir}/ser/modules/permissions.so
%{_libdir}/ser/modules/pike.so
%{_libdir}/ser/modules/print.so
%{_libdir}/ser/modules/registrar.so
%{_libdir}/ser/modules/rr.so
%{_libdir}/ser/modules/sl.so
%{_libdir}/ser/modules/sms.so
%{_libdir}/ser/modules/textops.so
%{_libdir}/ser/modules/tm.so
%{_libdir}/ser/modules/uri.so
%{_libdir}/ser/modules/usrloc.so
%{_libdir}/ser/modules/vm.so

%{_sbindir}/ser
%{_sbindir}/serctl

%{_bindir}/harv_ser.sh
%{_bindir}/gen_ha1

%{_mandir}/man5/*
%{_mandir}/man8/*


%files mysql
%defattr(-,root,root)

%{_libdir}/ser/modules/mysql.so
%{_sbindir}/ser_mysql.sh

%files jabber
%defattr(-,root,root)

%{_libdir}/ser/modules/jabber.so

%files radius
%defattr(-,root,root)

%{_libdir}/ser/modules/auth_radius.so
%{_libdir}/ser/modules/group_radius.so
%{_libdir}/ser/modules/uri_radius.so
* Tue Nov 12 2002 Andrei Pelinescu - Onciul <pelinescu-onciul@fokus.gmd.de>
- added a separate rpm for the jabber modules
- moved all the binaries to sbin
- removed obsolete installs (make install installs everything now)


%changelog

* Sun Jun 1 2003 Andrei Pelinescu - Onciul <pelinescu-onciul@fokus.fraunhofer.de>
- added a separate rpm for the radius modules
- updated to the new makefile variables (removed lots of unnecessary stuff)

* Thu Nov 14 2002 Jan Janak <J.Janak@sh.cvut.cz>
- Installing harv_ser.sh again
- quick hack to move gen_ha1 to bin directory instead of sbin (should
  be done from the Makefile next time)

* Tue Nov 12 2002 Andrei Pelinescu - Onciul <pelinescu-onciul@fokus.gmd.de>
- added a separate rpm for the jabber modules
- moved all the binaries to sbin
- removed obsolete installs (make install installs everything now)

* Fri Oct 25 2002 Jan Janak <J.Janak@sh.cvut.cz>
- Minor description fixes

* Fri Oct  4 2002 Jiri Kuthan <jiri@iptel.org>
- exec module introduced

* Wed Sep 25 2002 Andrei Pelinescu - Onciul  <pelinescu-onciul@fokus.gmd.de>
- modified make install & make: added cfg-target & modules-target

* Sun Sep 08 2002 Jan Janak <J.Janak@sh.cvut.cz>
- Created subpackage containing mysql connectivity support.

* Mon Sep 02 2002 Jan Janak <J.Janak@sh.cvut.cz>
- gen_ha1 utility added, scripts added.

* Tue Aug 28 2002 Jan Janak <J.Janak@sh.cvut.cz>
- Finished the first version of the spec file.

* Sun Aug 12 2002 Jan Janak <J.Janak@sh.cvut.cz>
- First version of the spec file.
