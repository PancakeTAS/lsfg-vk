{
  lib,
  SDL2,
  cmake,
  dxvk_2_git,
  git,
  glfw,
  glslang,
  gnused,
  hwdata,
  llvmPackages,
  meson,
  ninja,
  openssl,
  pe_parse_git,
  python3,
  sdl3,
  spirv-headers,
  vulkan-headers,
  vulkan-loader,
}:
llvmPackages.libcxxStdenv.mkDerivation rec {
  pname = "lsfg-vk";
  version = "0.0";

  src = ./.;

  nativeBuildInputs = [
    cmake
    git
    glslang
    gnused
    hwdata
    llvmPackages.clang-tools
    meson
    ninja
    openssl
    python3
    spirv-headers
    vulkan-headers
    vulkan-loader
  ];

  buildInputs = [
    SDL2
    glfw
    sdl3
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}/share/lsfg-vk"
    "-DCMAKE_CXX_FLAGS=-I${vulkan-headers}/include"
  ];

  patchPhase = ''
    # Use dxvk and pe-parse fetched by nix
    # This is needed because nix's sandboxing

    # It should be possible (and preferable) to use versions packaged by nix
    # to avoid recompiling but this should be fine for now
    mkdir -p build/dxvk/
    cp -r ${dxvk_2_git}/. build/dxvk/
    # cp -r ${dxvk_2_git}/.github build/dxvk/.git
    chmod -R +rwX build/dxvk

    mkdir -p build/pe_parse/
    cp -r ${pe_parse_git}/. build/pe_parse/
    chmod -R +w build/pe_parse

    sed -e 's,GIT_REPOSITORY.*,SOURCE_DIR "dxvk",'\
        -e 's,GIT_TAG.*,,'\
        -e 's,20602,20700,g'\
        -i cmake/FetchDXVK.cmake

    sed -e 's,GIT_REPOSITORY.*,SOURCE_DIR "pe_parse",'\
        -e 's,GIT_TAG.*,,' -i cmake/FetchPeParse.cmake

    sed -e 's,lsfg-vk-gen vulkan, lsfg-vk-gen,'\
        -i CMakeLists.txt

    patchShebangs build/dxvk/

    cat CMakeLists.txt
    cat cmake/FetchDXVK.cmake
  '';

  meta = {
    description = "This project brings Lossless Scaling's Frame Generation to Linux!";
    homepage = "https://github.com/PancakeTAS/lsfg-vk";
    license = with lib.licenses; [mit];
    mainProgram = "lsfg-vk";
    # maintainers = with lib.maintainers; [];
    platforms = [
      "x86_64-linux"
    ];
  };
}
