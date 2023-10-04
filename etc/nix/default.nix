# Copyright (C) 2023 Vincenzo Palazzo <vincenzopalazzodev@gmail.com>.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
with import <nixpkgs> { };
stdenv.mkDerivation {
  name = "poke-dev-nix-env";
  buildInputs = [
    gcc
    boehmgc
    readline
    dejagnu
    pkg-config
    autoconf-archive
    gnumake
    flex
    bison
    help2man
    autoconf
    automake
    libnbd
    gettext

    # optional dev dependencies
    ccache

    # debugging dependencies
    valgrind
  ];
}

