Name: lsfg-vk
Summary: Lossless Scaling Frame Generation on Linux via DXVK/Vulkan
Version: ${RPM_VERSION}
Release: 1%{?dist}
URL: https://discord.gg/losslessscaling
Packager: "PancakeTAS <???>"
License: MIT
Group: Applications/System
BuildArch: x86_64

Requires: glibc >= 2.38
Requires: vulkan-loader
Requires: libadwaita
Requires: gtk4

%global __strip /bin/true

%description
Lossless Scaling Frame Generation on Linux via DXVK/Vulkan.

%install
install -Dm755 %{_sourcedir}/lsfg-vk-ui %{buildroot}%{_bindir}/lsfg-vk-ui
install -Dm755 %{_sourcedir}/liblsfg-vk.so %{buildroot}%{_libdir}/liblsfg-vk.so
install -Dm644 %{_sourcedir}/VkLayer_LS_frame_generation.json %{buildroot}%{_datadir}/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
install -Dm644 %{_sourcedir}/lsfg-vk-ui.desktop %{buildroot}%{_datadir}/applications/lsfg-vk-ui.desktop
install -Dm644 %{_sourcedir}/gay.pancake.lsfg-vk-ui.png %{buildroot}%{_datadir}/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png

%files
%{_bindir}/lsfg-vk-ui
%{_libdir}/liblsfg-vk.so
%{_datadir}/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
%{_datadir}/applications/lsfg-vk-ui.desktop
%{_datadir}/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png
