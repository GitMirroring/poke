/* pkl-sir.h - Stack based IR for the poke compiler.  */

/* Copyright (C) 2024 Jose E. Marchesi.  */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PKL_SIR_H
#define PKL_SIR_H

#include <config.h>

/* The following enumeration defines the different kind of SIR
   instructions that compose a SIR program.  */

enum pkl_sir_insn_code
{
  /* Main stack manipulation instructions.  */
  PKL_SIR_PUSH,
  PKL_SIR_DROP,
  PKL_SIR_SWAP,
  PKL_SIR_NIP,
  PKL_SIR_DUP,
  PKL_SIR_OVER,
  PKL_SIR_OOVER,
  PKL_SIR_ROT,
  PKL_SIR_NROT,
  PKL_SIR_TUCK,
  PKL_SIR_QUAKE,
  PKL_SIR_REVN,
  /* Return stack manipulation instructions.  */
  PKL_SIR_SAVER,
  PKL_SIR_RESTORER,
  PKL_SIR_TOR,
  PKL_SIR_FROMR,
  PKL_SIR_ATR,
  /* Integer overflow checking instructions.  */
  PKL_SIR_ADDIOF,
  PKL_SIR_ADDLOF,
  PKL_SIR_SUBIOF,
  PKL_SIR_SUBLOF,
  PKL_SIR_MULIOF,
  PKL_SIR_MULLOF,
  PKL_SIR_DIVIOF,
  PKL_SIR_DIVLOF,
  PKL_SIR_MODIOF,
  PKL_SIR_MODLOF,
  PKL_SIR_NEGIOF,
  PKL_SIR_NEGLOF,
  PKL_SIR_POWIOF,
  PKL_SIR_POWLOF,
  /* Arithmetic instructions.  */
  PKL_SIR_ADDI,
  PKL_SIR_ADDIU,
  PKL_SIR_ADDL,
  PKL_SIR_ADDLU,
  PKL_SIR_SUBI,
  PKL_SIR_SUBIU,
  PKL_SIR_SUBL,
  PKL_SIR_SUBLU,
  PKL_SIR_MULI,
  PKL_SIR_MULIU,
  PKL_SIR_MULL,
  PKL_SIR_MULLU,
  PKL_SIR_DIVI,
  PKL_SIR_DIVIU,
  PKL_SIR_DIVL,
  PKL_SIR_DIVLU,
  PKL_SIR_MODI,
  PKL_SIR_MODIU,
  PKL_SIR_MODL,
  PKL_SIR_MODLU,
  PKL_SIR_NEGI,
  PKL_SIR_NEGIU,
  PKL_SIR_NEGL,
  PKL_SIR_NEGLU,
  PKL_SIR_POWI,
  PKL_SIR_POWIU,
  PKL_SIR_POWL,
  PKL_SIR_POWLU,
  /* Relational instructions.  */
  PKL_SIR_EQI,
  PKL_SIR_EQIU,
  PKL_SIR_EQL,
  PKL_SIR_EQLU,
  PKL_SIR_EQS,
  PKL_SIR_NEI,
  PKL_SIR_NEIU,
  PKL_SIR_NEL,
  PKL_SIR_NELU,
  PKL_SIR_NES,
  PKL_SIR_NN,
  PKL_SIR_NNN,
  PKL_SIR_LTI,
  PKL_SIR_LTIU,
  PKL_SIR_LTL,
  PKL_SIR_LTLU,
  PKL_SIR_LEI,
  PKL_SIR_LEIU,
  PKL_SIR_LEL,
  PKL_SIR_LELU,
  PKL_SIR_GTI,
  PKL_SIR_GTIU,
  PKL_SIR_GTL,
  PKL_SIR_GTLU,
  PKL_SIR_GEI,
  PKL_SIR_GEIU,
  PKL_SIR_GEL,
  PKL_SIR_GELU,
  /* Other instructions.  */
  PKL_SIR_EXIT,
  PKL_SIR_CANARY,
  PKL_SIR_PUSHEND,
  PKL_SIR_POPEND,
  PKL_SIR_PUSHOB,
  PKL_SIR_LAST
};

#endif /* ! PKL_SIR_H */
