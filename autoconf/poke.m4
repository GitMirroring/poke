# Autoconf macros for GNU poke.
# Copyright (C) 2023, 2024, 2025, 2026 Jose E. Marchesi

# This file is part of GNU poke.

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

# This macro defines the autoconf variable VARIABLE to 'poke' if the
# specified minimum version of poke is found in $PATH, or to ':'
# otherwise.
AC_DEFUN([PK_PROG_POKE], [
  AC_PATH_PROG([$1], [poke])
  if test -z "$[$1]"; then
    ac_verc_fail=yes
  else
    AC_MSG_CHECKING([for poke $2 or newer])
    # First see if poke is old enough to have pk_version.
    cat >conftest.pk <<_ACEOF
pk_version;
_ACEOF
    ac_poke_has_pk_version=no
    if $$1 -L conftest.pk 2>&1 >/dev/null; then
      ac_poke_has_pk_version=yes
    fi
    if test "x$ac_poke_has_pk_version" = "xyes"; then
      cat >conftest.pk <<_ACEOF
exit ((pk_vercmp (pk_version_parse (pk_version), pk_version_parse ("$2")) >= 0) ? 0 : 1);
_ACEOF
      ac_prog_version=`$$1 --version 2>&1 | sed -n 's/^.*GNU poke.* \(.*$\)/\1/p'`
      if $$1 -L conftest.pk 2>&1 >/dev/null; then
        ac_prog_version="$ac_prog_version, ok"
        ac_verc_fail=no
      else
        ac_prog_version="$ac_prog_version, bad"
        ac_verc_fail=yes
      fi
    else
      ac_prog_version="unknown"
    fi
    rm -f conftest.pk
    AC_MSG_RESULT([$ac_prog_version])
  fi
  if test $ac_verc_fail = yes; then
    [$1]=:
  fi
  AC_SUBST([$1])
])

# If POKE can load the given PICKLE, execute IF_FOUND, otherwise
# execute IF_NOT_FOUND.
AC_DEFUN([PK_CHECK_PICKLE], [
  cat >conftest.pk <<_ACEOF
load "$2";
_ACEOF
  AC_MSG_CHECKING([whether poke can load pickle $2])
  ac_pickle_found=
  if $1 -L conftest.pk 2>&1 >/dev/null; then
    ac_pickle_found=yes
    $3
  else
    ac_pickle_found=no
    $4
  fi
  rm -f conftest.pk
  AC_MSG_RESULT([$ac_pickle_found])
])
