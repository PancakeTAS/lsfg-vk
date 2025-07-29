{
  description = "A Vulkan layer for frame generation on Linux";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    self.submodules = true;
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        lsfg-vk-ui = pkgs.rustPlatform.buildRustPackage rec {
          pname = "lsfg-vk-ui";
          version = "0.9.0";

          src = ./ui;
          cargoLock.lockFile = ./ui/Cargo.lock;

          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.glib 
          ];
          buildInputs = [ pkgs.gtk4 pkgs.libadwaita ];
        };

        lsfg-vk-pkg = pkgs.stdenv.mkDerivation rec {
          pname = "lsfg-vk";
          version = "0.9.0";

          src = self;

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
            pkgs.clang
            pkgs.llvmPackages.bintools
          ];

          buildInputs = [
            pkgs.vulkan-loader
            pkgs.vulkan-headers
          ];

          configurePhase = ''
            runHook preConfigure
            cmake -S . -B build -G Ninja \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_C_COMPILER=clang \
              -DCMAKE_CXX_COMPILER=clang++ \
              -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=On
            runHook postConfigure
          '';

          buildPhase = ''
            runHook preBuild
            cmake --build build
            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall
            # Install the Vulkan layer from the CMake build
            cmake --install build --prefix $out

            # Install the pre-built Rust UI and its assets
            install -D ${lsfg-vk-ui}/bin/lsfg-vk-ui $out/bin/lsfg-vk-ui
            install -D $src/ui/rsc/gay.pancake.lsfg-vk-ui.desktop $out/share/applications/gay.pancake.lsfg-vk-ui.desktop
            install -D $src/ui/rsc/icon.png $out/share/icons/hicolor/256x256/apps/gay.pancake.lsfg-vk-ui.png
            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "Lossless Scaling Frame Generation Vulkan Layer and GUI";
            homepage = "https://github.com/PancakeTAS/lsfg-vk";
            license = licenses.mit;
            platforms = platforms.linux;
          };
        };
      in
      {
        packages.default = lsfg-vk-pkg;

        apps.default = {
          type = "app";
          program = "${lsfg-vk-pkg}/bin/lsfg-vk-ui";
        };

        devShells.default = pkgs.mkShell {
          name = "lsfg-vk-dev";
          inputsFrom = [ lsfg-vk-pkg lsfg-vk-ui ];
        };
      }
    );
}
