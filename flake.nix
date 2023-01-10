{
  description = "";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        with pkgs; 
        rec {
          defaultPackage = stdenv.mkDerivation {
            name = "chip8-cpp";
            src = ./.;
            nativeBuildInputs = [ 
              cmake gcc cppcheck clang-tools
            ];
            buildInputs = [
              SDL2
            ];
            cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];
            meta = with lib; {
              description = "Chip-8 interpreter in C++20";
              homepage = "https://github.com/delet-this/chip8-cpp";
              license = with licenses; [ mit ];
              maintainers = [ ];
              platforms = platforms.all;
              mainProgram = "chip8";
            };
          };

          apps.default = flake-utils.lib.mkApp { 
            drv = defaultPackage; 
            exePath = "/bin/chip8";
          };

          devShells.default = mkShell {
              buildInputs = with pkgs; [
                gcc gnumake cmake SDL2 cppcheck clang-tools
              ];

              shellHook = ''
                # ...
              '';
            };
        }
      );
}

