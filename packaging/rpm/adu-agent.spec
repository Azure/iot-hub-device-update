%global adu_user    adu
%global adu_group   adu
%global ext_dir     %{_libdir}/adu/extensions

Name:           adu-agent
Version:        2.0.0
Release:        1%{?dist}
Summary:        Azure Device Update agent
License:        MIT
URL:            https://github.com/Azure/adu-agent
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.14
BuildRequires:  ninja-build
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  libcurl-devel
BuildRequires:  openssl-devel
BuildRequires:  systemd-rpm-macros

Requires:       libcurl
Requires:       openssl-libs
Requires(pre):  shadow-utils

%description
The ADU Gen2 agent provides over-the-air update capabilities for
Linux-based IoT devices via the Azure Device Update service. It
supports modular extensions for communication providers, content
downloaders, step handlers, and content processors.

# ── Extension subpackages ────────────────────────────────────────────

%package -n adu-ext-adu-direct
Summary:    ADU Direct communication provider extension
Requires:   %{name} >= 2.0.0

%description -n adu-ext-adu-direct
Provides the ADU Direct communication provider for the Azure Device
Update agent, enabling direct communication with the ADU service.

%package -n adu-ext-simulator
Summary:    Simulator communication provider extension
Requires:   %{name} >= 2.0.0

%description -n adu-ext-simulator
Provides a simulator communication provider for the Azure Device
Update agent, useful for testing and development.

%package -n adu-ext-curl-downloader
Summary:    cURL content downloader extension
Requires:   %{name} >= 2.0.0
Requires:   libcurl

%description -n adu-ext-curl-downloader
Provides the cURL-based content downloader for the Azure Device
Update agent.

%package -n adu-ext-sideload-downloader
Summary:    Sideload content downloader extension
Requires:   %{name} >= 2.0.0

%description -n adu-ext-sideload-downloader
Provides a local file sideload downloader for the Azure Device
Update agent, enabling updates from local storage.

%package -n adu-ext-script-handler
Summary:    Script step handler extension
Requires:   %{name} >= 2.0.0

%description -n adu-ext-script-handler
Provides the script-based step handler for the Azure Device
Update agent, enabling custom update logic via shell scripts.

%package -n adu-ext-swupdate-handler
Summary:    SWUpdate step handler extension
Requires:   %{name} >= 2.0.0

%description -n adu-ext-swupdate-handler
Provides the SWUpdate-based step handler for the Azure Device
Update agent, enabling integration with the SWUpdate framework.

%package -n adu-ext-delta-processor
Summary:    Delta content processor extension
Requires:   %{name} >= 2.0.0

%description -n adu-ext-delta-processor
Provides the delta content processor for the Azure Device
Update agent, enabling differential/delta updates.

%package -n adu-diagnostics
Summary:    ADU agent diagnostic tools
Requires:   %{name} >= 2.0.0

%description -n adu-diagnostics
Provides diagnostic and troubleshooting tools for the Azure Device
Update agent, including the status CLI and log decoder utilities.

%package -n adu-edk-devel
Summary:    ADU Extension Development Kit (EDK)
Requires:   %{name} = %{version}-%{release}

%description -n adu-edk-devel
Provides headers, CMake config files, pkg-config metadata, and
starter templates for developing custom extensions for the Azure
Device Update agent.

# ── Build ────────────────────────────────────────────────────────────

%prep
%autosetup -n %{name}-%{version}

%build
%cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DADUC_BUILD_WITH_DELIVERY_OPTIMIZATION=OFF \
    -DADUC_BUILD_UNIT_TESTS=OFF
%cmake_build

%install
%cmake_install

# Install systemd service
install -D -m 0644 packaging/debian/adu-agent.service \
    %{buildroot}%{_unitdir}/adu-agent.service

# Ensure extension directory exists
install -d %{buildroot}%{ext_dir}

# Ensure config directory and default config
install -d %{buildroot}%{_sysconfdir}/adu
install -d %{buildroot}%{_sysconfdir}/adu/extensions.d

# Ensure man page directories
install -d %{buildroot}%{_mandir}/man1

# ── Scripts ──────────────────────────────────────────────────────────

%pre
getent group %{adu_group} >/dev/null || groupadd -r %{adu_group}
getent passwd %{adu_user} >/dev/null || \
    useradd -r -g %{adu_group} -d /var/lib/adu -s /sbin/nologin \
    -c "Azure Device Update Agent" %{adu_user}
exit 0

%post
%systemd_post adu-agent.service

install -d -m 0750 -o %{adu_user} -g %{adu_group} /var/lib/adu
install -d -m 0750 -o %{adu_user} -g %{adu_group} /var/lib/adu/downloads
install -d -m 0750 -o %{adu_user} -g %{adu_group} /var/log/adu
install -d -m 0750 -o %{adu_user} -g %{adu_group} /etc/adu/extensions.d
install -d -m 0750 -o %{adu_user} -g %{adu_group} %{ext_dir}

%preun
%systemd_preun adu-agent.service

%postun
%systemd_postun_with_restart adu-agent.service

# ── File lists ───────────────────────────────────────────────────────

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/adu_agent
%{_unitdir}/adu-agent.service
%dir %{_sysconfdir}/adu
%config(noreplace) %{_sysconfdir}/adu/agent.toml
%dir %{_sysconfdir}/adu/extensions.d
%dir %{ext_dir}

%files -n adu-ext-adu-direct
%{ext_dir}/libadu_direct_comm.so

%files -n adu-ext-simulator
%{ext_dir}/libsimulator_comm.so

%files -n adu-ext-curl-downloader
%{ext_dir}/libcurl_downloader.so

%files -n adu-ext-sideload-downloader
%{ext_dir}/libsideload_downloader.so

%files -n adu-ext-script-handler
%{ext_dir}/libscript_handler.so

%files -n adu-ext-swupdate-handler
%{ext_dir}/libswupdate_handler.so

%files -n adu-ext-delta-processor
%{ext_dir}/libdelta_processor.so

%files -n adu-diagnostics
%{_bindir}/adu_status_cli
%{_bindir}/adu_rc_decoder
%{_mandir}/man1/adu_status_cli.1*
%{_mandir}/man1/adu_rc_decoder.1*

%files -n adu-edk-devel
%{_includedir}/aduc/
%{_libdir}/cmake/ADUCEDK/
%{_libdir}/pkgconfig/aducedk.pc
%{_datadir}/adu-edk/templates/

%changelog
* Mon Jan 01 2024 Microsoft Corporation <aduagent@microsoft.com> - 2.0.0-1
- Initial RPM package for ADU Gen2 agent
- Modular extension architecture with communication providers,
  content downloaders, step handlers, and content processors
- Separate subpackages for each extension and development kit
