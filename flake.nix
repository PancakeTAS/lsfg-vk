{
  description = "Lsfg-vk nix flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";

    dxvk_2_git = {
      url = "git+https://github.com/doitsujin/dxvk?submodules=1";
      flake = false;
    };
    pe_parse_git = {
      url = "github:trailofbits/pe-parse";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    dxvk_2_git,
    pe_parse_git,
    ...
  }: let
    supportedSystems = [
      "x86_64-linux"
    ];

    forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
  in {
    packages = forAllSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      default = pkgs.callPackage ./default.nix {
        inherit dxvk_2_git;
        inherit pe_parse_git;
      };
    });

    devShells = forAllSystems (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        default = pkgs.mkShell {
          buildInputs = with pkgs; [git rsync hugo];
        };
      }
    );
  };
}
