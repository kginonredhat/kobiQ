Name:           kobiq
Version:        0.1.7
Release:        1%{?dist}
Summary:        kobiQ clipboard manager for Linux

License:        MIT
URL:            https://github.com/kginonredhat/kobiQ
Source0:        %{url}/archive/refs/tags/v%{version}.tar.gz#/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
Requires:       qt6-qtbase
Requires:       qt6-qtbase-gui
Requires:       python3
Recommends:     xdotool
Recommends:     wl-clipboard

%description
kobiQ is a local-first clipboard manager for Linux. It stores a searchable
history of text and images, provides a system tray UI, and keeps all data on
your machine.

Build a separate RPM for each Fedora release (42/43/44). Qt ABI differs
across Fedora versions, so an .fc44 package is not installable on Fedora 42.

After install, set a GNOME custom shortcut to: kobiq-activate

%prep
%autosetup -n kobiQ-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/kobiQ
%{_bindir}/kobiq-show
%{_bindir}/kobiq-show-debug
%{_bindir}/kobiq-activate
%{_bindir}/kobiq-enable-gnome-monitor
%{_datadir}/applications/kobiQ.desktop
%{_datadir}/applications/org.kobiq.Application.desktop
%{_datadir}/icons/hicolor/scalable/apps/kobiQ.svg
%{_datadir}/gnome-shell/extensions/kobiq-clipboard-monitor@kobiq.org/
%{_metainfodir}/kobiQ.metainfo.xml

%changelog
* Wed Oct 07 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.7-1
- GNOME Wayland clipboard monitoring via Shell extension and GPaste fallback

* Thu Aug 13 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.6-1
- Also copy restored items to Primary Selection (middle-click paste)

* Wed Aug 12 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.4-1
- Wayland show/hide fixes, D-Bus activation, kobiq-activate shortcut helper

* Tue Aug 11 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.3-1
- Fix Ctrl+~ opening duplicate instances (single-instance socket handling)

* Tue Aug 11 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.2-1
- Exit menu, tray fixes, multi-Fedora RPM builds (42/43/44)

* Tue Aug 11 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.1-1
- Fix hide-to-tray / close behavior and show window on launch

* Tue Aug 11 2026 kginonredhat <kginonredhat@users.noreply.github.com> - 0.1.0-1
- Initial RPM package for kobiQ 0.1.0
