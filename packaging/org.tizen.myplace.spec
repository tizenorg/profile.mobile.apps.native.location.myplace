Name:       org.tizen.myplace
Summary:    My places
Version:    1.0.0
Release:    1
Group:      Applications/Location
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(ui-gadget-1)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(capi-location-manager)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(capi-geofence-manager)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: edje-bin
BuildRequires: cmake
BuildRequires: gettext-tools
Requires(post):   /sbin/ldconfig
Requires(post):   /usr/bin/vconftool
requires(postun): /sbin/ldconfig

%define appdir /usr/apps/org.tizen.myplace

%description
Manage places for geofence service.


%prep
%setup -q

%build
%define PREFIX    "/usr/apps/org.tizen.myplace"
%define RW_PREFIX	"/opt/usr/apps/org.tizen.myplace"

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DCMAKE_INSTALL_RW_PREFIX=%{RW_PREFIX}

make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}

%make_install


%post
/sbin/ldconfig

#/usr/bin/vconftool set -t int file/private/myplace/geofence 1 -g 5000 -f -s org.tizen.myplace

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.myplace.manifest
/etc/smack/accesses.d/org.tizen.myplace.efl
%defattr(-,root,root,-)
%{appdir}/*
%defattr(-,root,root,757)
/usr/share/packages/org.tizen.myplace.xml

%defattr(-,app,app,-)
